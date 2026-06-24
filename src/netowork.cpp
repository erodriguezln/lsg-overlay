//
// Created by Mathw on 24/6/2026.
//

#include "netowork.h"

Network::Network() : client("http://127.0.0.1:3333") {
    client.set_connection_timeout(5);
    client.set_read_timeout(10);
}

Network::~Network() {
}

bool Network::fetchMechanics() {
    auto response = client.Get("/videogames/12/mechanics");
    if (response && response->status == 200) {
        auto json = nlohmann::json::parse(response->body);
        for (auto &m: json) {
            Mechanic mechanic;
            mechanic.id_modifiable_mechanic_videogame = m["id_modifiable_mechanic_videogame"];
            mechanic.id_videogame = m["id_videogame"];
            mechanic.options = m["options"];
            mechanic.id_modifiable_mechanic = m["id_modifiable_mechanic"];
            mechanic.name = m["modifiable_mechanic_name"];
            mechanic.description = m["modifiable_mechanic_description"];
            mechanic.type = m["modifiable_mechanic_type"];
            mechanics.push_back(mechanic);
        }
        return true;
    }
    std::cout << "Failed to fetch mechanics: " << (response
                                                       ? std::to_string(response->status)
                                                       : "No response from server") << std::endl;
    return false;
}

const std::vector<Mechanic> &Network::getMechanics() const {
    return mechanics;
}

void Network::printMechanics() const {
    for (const Mechanic &mechanic: mechanics) {
        std::cout << "ID: " << mechanic.id_modifiable_mechanic << '\n';
        std::cout << "Name: " << mechanic.name << '\n';
        std::cout << "Description: " << mechanic.description << '\n';
        std::cout << "Type: " << mechanic.type << '\n';
        std::cout << "Options: " << mechanic.options.dump(4) << '\n';
        std::cout << "------------------------\n";
    }
}
