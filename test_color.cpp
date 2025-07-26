struct TestColor { glm::vec4 color{0.35f, 0.47f, 0.78f, 1.0f}; };
#include "Core/Utils/XMLSerialization.hpp"
#include <iostream>
int main() { TestColor tc; pugi::xml_document doc; auto root = doc.append_child("test"); XML::auto_serialize(tc, root); doc.save_file("color_test.xml"); std::cout << "Color test saved
"; return 0; }
