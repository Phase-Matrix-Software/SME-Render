#include "SME_xml.h"
#include <SME_util.h>
#include <fstream>
#include <iostream>

/*
 * Returns the index of either the first / or > after the attributes
 */
int getEndOfAttribsIndex(std::string str) {
    bool inStr = false;
    for (int i = 0; char c = str[i]; i++) {
        if (c == '"') inStr = !inStr;
        if (!inStr && (c == '/' || c == '>')) {
            return i;
        }
    }
}

SME::XML::Tag SME::XML::parseXML(std::string path) {
    std::ifstream in(path);

    SME::XML::Tag base, *current = &base;
    base.name = "base";

    std::string line;
    std::string xml;
    while (std::getline(in, line)) {
        xml += line;
    }

    while (xml.find('<') != std::string::npos) {
        if (xml.at(1) == '?') { //skip any 
            xml = xml.substr(xml.find('>')).substr(xml.find('<') + 1);
            continue;
        }
        xml = xml.substr(xml.find('<') + 1);

        if (xml.at(0) == '/') {//close tag
            current = current->parent;
        } else { //open tag
            Tag newTag;
            newTag.parent = current;
            newTag.name = xml.substr(0, xml.find_first_of(" >"));
            newTag.contents = xml.substr(xml.find('>') + 1, xml.find('<') - xml.find('>') - 1);
            int attribsIndex = getEndOfAttribsIndex(xml);
            if (attribsIndex > xml.find(' ')) {
                std::string attributes = xml.substr(xml.find(' '), attribsIndex - xml.find(' ')); //TODO "/>" is matching with strings (ie urls)
                for (std::string attrib : SME::Util::split(attributes, ' ', true)) { //for each attribute, ignoring any empty entries
                    std::vector<std::string> split = SME::Util::split(attrib, '=');
                    std::string key = split[0];
                    std::string val = split[1];
                    newTag.attributes[key] = val;
                }
            }

            current->children.push_back(newTag);
            if (xml.substr(attribsIndex, xml.find('>', attribsIndex)-attribsIndex).find('/') == std::string::npos) { // not closing tag (<example />)
                current = &current->children.back();
            }
        }
    }
    return base;
}

SME::XML::Tag SME::XML::getFirstTag(Tag tag, std::string retTag) {
    std::vector<std::string> split = SME::Util::split(retTag, '.');

    for (std::string name : split) {
        bool match = false;
        for (Tag t : tag.children) {
            if (t.name == name) {
                tag = t;
                match = true;
                break;
            }
        }
        if (!match) {
            std::cout << "No matching Tag found" << std::endl;
            exit(-1);
        }
    }
    return tag;
}