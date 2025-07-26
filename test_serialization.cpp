#include "Core/Utils/XMLSerialization.hpp"
#include <iostream>

struct TestData {
    int id = 42;
    float value = 3.14f;
    std::string name = "example";
    glm::vec2 position{10.0f, 20.0f};
};

int main() {
    TestData data;
    
    pugi::xml_document doc;
    auto root = doc.append_child("test");
    
    XML::auto_serialize(data, root);
    
    doc.save_file("test_output.xml");
    
    std::cout << "Automatyczna serializacja zakończona! Sprawdź plik test_output.xml\n";
    
    TestData loaded;
    pugi::xml_document loadDoc;
    if (loadDoc.load_file("test_output.xml")) {
        XML::auto_deserialize(loaded, loadDoc.child("test"));
        std::cout << "Załadowano: id=" << loaded.id << ", value=" << loaded.value 
                  << ", name=" << loaded.name << ", pos=(" << loaded.position.x 
                  << "," << loaded.position.y << ")\n";
    }
    
    return 0;
}
