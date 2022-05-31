#include <random>
#include <string>
#include <list>
#include "pioche.h"
#include "tuile.h"
#include "element.h"

#include "../utils/libraries/tinyxml2.h"


Pioche* Pioche::instance = nullptr;

Pioche &Pioche::getInstance() {
    if (nullptr == instance) {
        instance = new Pioche();
    }
    return *instance;
}

void Pioche::libereInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

Pioche::~Pioche() = default;

Tuile* Pioche::piocher() {
    srand(time(NULL));
    int random;
    Tuile *t;

    if (!tuiles_riviere.empty()) { // si on a la riviere on pioche dedans
        int last_tile = tuiles_riviere.back()->getID(); // on veut piocher le lac en dernier
        if (tuiles_riviere.size() == nbTuilesRiviereMax) { // si la pioche de riviere est pleine on prend la source
            t = tuiles_riviere.front();
            tuiles_riviere.erase(tuiles_riviere.begin());
        } else { // si elle est pas pleine on prend au hasard
            random = rand() % tuiles_riviere.size();
            t = tuiles_riviere[random];
            while (t->getID() == last_tile && tuiles_riviere.size() != 1) { // verifier qu'on a pas pioché le lac
                random = rand() % tuiles_riviere.size();
                t = tuiles_riviere[random];
            }
            tuiles_riviere.erase(tuiles_riviere.begin() + random);
        }
    } else { // sinon on pioche dans les tuiles normales
        if (nbTuilesRiviereMax == 0 && tuiles.size() == nbTuilesMax) { // si y'a pas la rivière faut piocher la tuile de départ
            t = tuiles[3];
            tuiles.erase(tuiles.begin()+3);
        } else { // sinon on prend une tuile au hasard
            random = rand() % tuiles.size();
            t = tuiles[random];
            tuiles.erase(tuiles.begin()+random);
        }
    }
    return t;
    /*
    if (!tuiles.empty()) {
        srand(time(NULL));
        int random = rand() % tuiles.size();
        Tuile* selected = tuiles[random];
        tuiles.erase(tuiles.begin() + random);
        return selected;
    }
     */
}

using namespace tinyxml2;

TypeCardinaux::points parseOrientation(std::string orientation) {
    if (orientation == "N") {
        return TypeCardinaux::points::nord;
    } else if (orientation == "NE") {
        return TypeCardinaux::points::nord_est;
    } else if (orientation == "E") {
        return TypeCardinaux::points::est;
    } else if (orientation == "SE") {
        return TypeCardinaux::points::sud_est;
    } else if (orientation == "S") {
        return TypeCardinaux::points::sud;
    } else if (orientation == "SO") {
        return TypeCardinaux::points::sud_ouest;
    } else if (orientation == "O") {
        return TypeCardinaux::points::ouest;
    } else if (orientation == "NO") {
        return TypeCardinaux::points::nord_ouest;
    }
}

Element* parseElem(XMLNode* element) {
    std::string type = element->FirstChildElement("type")->GetText();;

    Element *e;

    // elements sans orientations
    if (type == "jardin") {
        e = new ElementJardin();
        return e;
    } else if (type == "abbaye") {
        e = new ElementAbbaye();
        return e;
    } else { // elements avec orientations
        // parsing des orientations
        size_t nb_orientations;
        size_t orientation_size;
        std::string orientations = element->FirstChildElement("orientation")->GetText();
        if (type == "pre") {
            orientation_size = 2;
            nb_orientations = (orientations.length() + 1) / 3;
        } else {
            orientation_size = 1;
            nb_orientations = (orientations.length() + 1) / 2;
        }
        std::list<TypeCardinaux::points> final_orientations;
        for (size_t i = 0; i < nb_orientations; i++) {
            std::string str_orientation = orientations.substr(i * (orientation_size + 1), orientation_size);
            TypeCardinaux::points orientation = parseOrientation(str_orientation);
            final_orientations.push_back(orientation);
        }
        if (type == "ville") {
            bool blason, cathedrale;

            if (element->FirstChildElement("blason") != nullptr) {
                blason = true;
            } else {
                blason = false;
            }

            if (element->FirstChildElement("cathedrale") != nullptr) {
                cathedrale = true;
            } else {
                cathedrale = false;
            }

            e = new ElementVille(final_orientations, blason, cathedrale);
            return e;
        } else if (type == "chemin") {
            bool auberge;
            if (element->FirstChildElement("auberge") != nullptr) {
                auberge = strcmp(element->FirstChildElement("auberge")->GetText(), "true") == 0;
            } else {
                auberge = false;
            }
            e = new ElementRoute(final_orientations, auberge);
            return e;
        } else if (type == "pre") {
            e = new ElementPre(final_orientations);
            return e;
        } else if (type == "riviere") {
            e = new ElementRiviere(final_orientations);
            return e;
        } else {
            std::cout << "Erreur : type d'element inconnu" << std::endl;
        }
    }
}

void Pioche::genererTuiles(std::list<std::string> extensions) {
    // loading XML file
    std::string path;
    for (auto it = extensions.begin(); it != extensions.end(); it++) {
        XMLDocument tiles;
        std::vector<Tuile*> destination;
        if ((*it) == "main") {
            tiles.LoadFile("../utils/tuiles-main.xml");
        } else if ((*it) == "riviere") {
            tiles.LoadFile("../utils/tuiles-riviere.xml");
        } else if ((*it) == "auberge") {
            tiles.LoadFile("../utils/tuiles-auberges.xml");
        } else {
            std::cout << "Pas de tuiles à générer pour l'extension : " << (*it) << std::endl;
            return;
        }
        XMLElement *root = tiles.FirstChildElement("tuiles");
        XMLElement *tuile = root->FirstChildElement("tuile");

        while (tuile != nullptr) {
            int tuile_id = atoi(tuile->Attribute("id"));

            std::list<Element *> elements;
            XMLElement *element = tuile->FirstChildElement("element");
            while (element != nullptr) {
                Element *e = parseElem(element);
                // creating the element
                elements.push_back(e);
                element = element->NextSiblingElement("element");
            }
            // creating the tuile
            Tuile *t = new Tuile(tuile_id, elements);
            if ((*it) == "riviere") {
                tuiles_riviere.push_back(t);
                nbTuilesRiviereMax++;
            } else {
                tuiles.push_back(t);
                nbTuilesMax++;
            }
            tuile = tuile->NextSiblingElement("tuile");
        }
    }
}
