/* 
 * File:   SME_xml.h
 * Author: Sam
 *
 * Created on 22 June 2016, 16:35
 */

#ifndef SME_XML_H
#define	SME_XML_H

#include <vector>
#include <string>
#include <map>

namespace SME {
    namespace XML {
        struct Tag {
            Tag *parent = nullptr;
            std::string name;
            std::map<std::string, std::string> attributes;
            std::vector<Tag> children;
            std::string contents;
       };
       
       Tag parseXML(std::string path);
       Tag getFirstTag(Tag tag, std::string retTag);
    }
}
#endif	/* SME_XML_H */

