// Teleplot (Windows version)
// Source: https://github.com/nesnes/teleplot
// Patched for WinSock2 and MSVC (no Linux headers required)

#ifndef TELEPLOT_H
#define TELEPLOT_H

#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <map>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#define TELEPLOT_FLAG_DEFAULT ""
#define TELEPLOT_FLAG_NOPLOT "np"
#define TELEPLOT_FLAG_2D "xy"
#define TELEPLOT_FLAG_TEXT "text"

class ShapeTeleplot {
public:
    class ShapeValue {
    public:
        ShapeValue() : isSet(false), value(0) {};
        ShapeValue(double val) : isSet(true), value(val) {};

        bool isSet;
        double value;

        std::string valueRounded() const {
            return roundValue(value, 3);
        }

    private:
        std::string roundValue(const double value, const unsigned short precision) const {
            std::string value_str = std::to_string(value);
            int res_length = value_str.length();

            int i = 0;
            bool stop = false;

            while (i < res_length && !stop) {
                if (value_str[i] == '.') {
                    int u = i + precision;
                    if (u + 1 < value_str.length()) {
                        while (value_str[u] == '0') u--;

                        res_length = u;
                        if (i != u)
                            res_length++;
                    }
                    stop = true;
                }
                i++;
            }

            return value_str.substr(0, res_length);
        }
    };

    ShapeTeleplot() {};

    ShapeTeleplot(std::string const& name, std::string const& type, std::string const& color = "")
        : _name(name), _type(type), _color(color) {};

    std::string const& getName() const {
        return _name;
    }

    ShapeTeleplot& setPos(ShapeValue const posX, ShapeValue const posY = {}, ShapeValue const posZ = {}) {
        _posX = posX; _posY = posY; _posZ = posZ; return *this;
    }

    ShapeTeleplot& setRot(ShapeValue const rotX, ShapeValue const rotY = {}, ShapeValue const rotZ = {}, ShapeValue const rotW = {}) {
        _rotX = rotX; _rotY = rotY; _rotZ = rotZ; _rotW = rotW; return *this;
    }

    ShapeTeleplot& setCubeProperties(ShapeValue const height, ShapeValue const width = {}, ShapeValue const depth = {}) {
        _height = height; _width = width; _depth = depth; return *this;
    }

    ShapeTeleplot& setSphereProperties(ShapeValue const radius, ShapeValue const precision) {
        _radius = radius; _precision = precision; return *this;
    }

    std::string toString() const {
        std::stringstream result;
        result << std::fixed << std::setprecision(4);
        result << "S:" << _type;

        if (_color != "") { result << ":C:" << _color; }

        if (_posX.isSet || _posY.isSet || _posZ.isSet) {
            result << ":P:";
            if (_posX.isSet) { result << _posX.valueRounded(); }
            result << ":";
            if (_posY.isSet) { result << _posY.valueRounded(); }
            result << ":";
            if (_posZ.isSet) { result << _posZ.valueRounded(); }
        }

        if (_rotX.isSet || _rotY.isSet || _rotZ.isSet || _rotW.isSet) {
            if (_rotW.isSet) { result << ":Q:"; }
            else { result << ":R:"; }

            if (_rotX.isSet) { result << _rotX.valueRounded(); }
            result << ":";

            if (_rotY.isSet) { result << _rotY.valueRounded(); }
            result << ":";

            if (_rotZ.isSet) { result << _rotZ.valueRounded(); }

            if (_rotW.isSet) { result << ":" << _rotW.valueRounded(); }
        }

        if (_type == "sphere") {
            if (_radius.isSet) { result << ":RA:" << _radius.valueRounded(); }
            if (_precision.isSet) { result << ":P:" << _precision.valueRounded(); }
        }

        if (_type == "cube") {
            if (_height.isSet) { result << ":H:" << _height.valueRounded(); }
            if (_width.isSet) { result << ":W:" << _width.valueRounded(); }
            if (_depth.isSet) { result << ":D:" << _depth.valueRounded(); }
        }

        return result.str();
    }

private:
    const std::string _name;
    const std::string _type;
    const std::string _color;

    ShapeValue _posX, _posY, _posZ;
    ShapeValue _rotX, _rotY, _rotZ, _rotW;
    ShapeValue _height, _width, _depth;
    ShapeValue _radius, _precision;
};

class Teleplot {
public:
    Teleplot(std::string address, unsigned int port = 47269, unsigned int bufferingFrequencyHz = 30)
        : address_(address), bufferingFrequencyHz_(bufferingFrequencyHz) 
    {
#pragma comment(lib, "ws2_32.lib")

        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&serv_, 0, sizeof(serv_));
        serv_.sin_family = AF_INET;
        serv_.sin_port = htons(port);
        //serv_.sin_addr.s_addr = inet_addr(address_.c_str());

        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        addrinfo* result = nullptr;
        int res = getaddrinfo(address_.c_str(), nullptr, &hints, &result);
        if (res != 0 || !result)
        {
            // Fallback to direct IP if needed
            serv_.sin_addr.s_addr = inet_addr(address_.c_str());
        }
        else
        {
            sockaddr_in* ipv4 = (sockaddr_in*)result->ai_addr;
            serv_.sin_addr = ipv4->sin_addr;
        }

        freeaddrinfo(result);
        #pragma comment(lib, "ws2_32.lib")

    }

    ~Teleplot() = default;

    static Teleplot &localhost() {
        static Teleplot teleplot("127.0.0.1");
        return teleplot;
    }

    template<typename T>
    void update(std::string const& key, T const& value, std::string unit = "") {
        int64_t nowUs = std::chrono::time_point_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now()
                        ).time_since_epoch().count();
        double nowMs = static_cast<double>(nowUs) / 1000.0;
        updateData(key, nowMs, value, 0, unit);
    }

    template<typename T1, typename T2>
    void update2D(std::string const& key, T1 const& x, T2 const& y) {
        int64_t nowUs = std::chrono::time_point_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now()
                        ).time_since_epoch().count();
        double nowMs = static_cast<double>(nowUs) / 1000.0;
        updateData(key, x, y, nowMs, TELEPLOT_FLAG_2D);
    }

    void log(std::string const& logmsg) {
        int64_t nowMs = std::chrono::time_point_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now()
                        ).time_since_epoch().count();
        emit(">" + std::to_string(nowMs) + ":" + logmsg);
    }

private:

    template<typename T1, typename T2, typename T3>
    void updateData(std::string const& key, T1 const& valueX, T2 const& valueY, T3 const& valueZ, std::string const& flags) {
        std::ostringstream oss;
        oss << std::fixed << valueX << ":" << valueY;
        if (flags == TELEPLOT_FLAG_2D)
            oss << ":" << valueZ;
        std::string data = formatPacket(key, oss.str(), flags);
        emit(data);
    }

    std::string formatPacket(std::string const& key, std::string const& values, std::string const& flags) {
        std::ostringstream oss;
        oss << key << ":" << values << "|" << flags;
        return oss.str();
    }

    void emit(std::string const& data) {
        sendto(sockfd_, data.c_str(), (int)data.size(), 0,
            (sockaddr*)&serv_, sizeof(serv_));
    }

    int sockfd_;
    std::string address_;
    sockaddr_in serv_;
    unsigned int bufferingFrequencyHz_;
};

#endif
