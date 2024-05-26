#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace transport_commands {
    enum class Type {
        STOP,
        BUS
    };

    struct Description {
        // Определяет, задана ли команда (поле command непустое)
        explicit operator bool() const {
            return !command.empty();
        }

        bool operator!() const {
            return !operator bool();
        }

        std::string command;      // Название команды
        std::string id;           // id маршрута или остановки
        std::string description;  // Параметры команды
    };

    class InputReader {
    public:
        /**
         * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
         */
        void ParseLine(std::string_view line);

        /**
         * Наполняет данными транспортный справочник, используя команды из commands_
         */
        void ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const;

    private:
        std::unordered_map<Type, std::vector<Description>> commands_;
    };

    namespace detail {
        Coordinates ParseCoordinates(std::string_view str);
        std::string_view Trim(std::string_view string);
        std::vector<std::string_view> Split(std::string_view string, char delim);
        std::vector<std::string_view> ParseRoute(std::string_view route);
        Description ParseDescription(std::string_view line);

    }
}
