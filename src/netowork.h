//
// Created by Mathw on 24/6/2026.
//

#ifndef LSG_OVERLAY_NETWORK_H
#define LSG_OVERLAY_NETWORK_H

#include <httplib.h>
#include <nlohmann/json.hpp>

struct Mechanic {
    int id_modifiable_mechanic_videogame;
    int id_videogame;
    nlohmann::json options;
    int id_modifiable_mechanic;
    std::string name;
    std::string description;
    std::string type;
};


class Network {
public:
    Network();

    ~Network();

    bool fetchMechanics();

    const std::vector<Mechanic>& getMechanics() const;

    void printMechanics() const;

    std::vector<Mechanic> mechanics;

private:
    httplib::Client client;
};


#endif //LSG_OVERLAY_NETWORK_H
