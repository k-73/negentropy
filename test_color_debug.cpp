#include "Core/Utils/XMLSerialization.hpp"
#include <iostream>

struct TestColor {
    glm::vec4 color{0.35f, 0.47f, 0.78f, 1.0f};
};

int main() {
    TestColor tc;
    pugi::xml_document doc;
    auto root = doc.append_child("test");
    
    XML::auto_serialize(tc, root);
    doc.save_file("color_test.xml");
    
    std::cout << "Original color: " << tc.color.r << " " << tc.color.g << " " << tc.color.b << " " << tc.color.a << "\n";
    
    TestColor loaded;
    pugi::xml_document loadDoc;
    if (loadDoc.load_file("color_test.xml")) {
        XML::auto_deserialize(loaded, loadDoc.child("test"));
        std::cout << "Loaded color: " << loaded.color.r << " " << loaded.color.g << " " << loaded.color.b << " " << loaded.color.a << "\n";
    }
    
    return 0;
}
