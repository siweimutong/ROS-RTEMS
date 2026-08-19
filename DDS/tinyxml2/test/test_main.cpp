// test_tinyxml2_simple.cpp
// Compile: arm-rtems5-g++ test_tinyxml2_simple.cpp -I/path/to/tinyxml2/include -L/path/to/tinyxml2/lib -ltinyxml2 -o test_xml
// Run: ./test_xml

#include <iostream>
#include "tinyxml2.h"
#include <rtems.h>

using namespace tinyxml2;
using namespace std;

extern "C" rtems_task Init(
  rtems_task_argument ignored
) 
{
    // 1. Test document parsing
    cout << "=== Test 1: Parse an XML string ===" << endl;
    XMLDocument doc;
    const char* xml = "<?xml version=\"1.0\"?>"
                      "<root>"
                      "  <person id=\"1\">"
                      "    <name>John Doe</name>"
                      "    <age>30</age>"
                      "  </person>"
                      "  <person id=\"2\">"
                      "    <name>Jane Smith</name>"
                      "    <age>25</age>"
                      "  </person>"
                      "</root>";
    
    XMLError error = doc.Parse(xml);
    if (error != XML_SUCCESS) {
        cout << "Parse failed! Error code: " << error << endl;
        exit(1);
    }
    cout << "XML parsed successfully!" << endl;
    
    // 2. Test the XMLNode::Value() function (the error you encountered)
    cout << "\n=== Test 2: Test XMLNode::Value() ===" << endl;
    XMLNode* root = doc.FirstChild();
    if (root) {
        const char* rootName = root->Value();
        cout << "Root node name: " << (rootName ? rootName : "(empty)") << endl;
    }
    
    // 3. Test XMLElement::GetText()
    cout << "\n=== Test 3: Test XMLElement::GetText() ===" << endl;
    XMLElement* firstPerson = doc.FirstChildElement("root")->FirstChildElement("person");
    if (firstPerson) {
        XMLElement* nameElement = firstPerson->FirstChildElement("name");
        if (nameElement) {
            const char* nameText = nameElement->GetText();
            cout << "First person name: " << (nameText ? nameText : "(empty)") << endl;
        }
    }
    
    // 4. Iterate over all person nodes
    cout << "\n=== Test 4: Iterate over all nodes ===" << endl;
    XMLElement* person = doc.FirstChildElement("root")->FirstChildElement("person");
    int count = 0;
    while (person) {
        const char* id = person->Attribute("id");
        const char* name = person->FirstChildElement("name")->GetText();
        const char* age = person->FirstChildElement("age")->GetText();
        
        cout << "Person " << ++count << ": ID=" << (id ? id : "unknown")
             << ", Name=" << (name ? name : "unknown")
             << ", Age=" << (age ? age : "unknown") << endl;
        
        person = person->NextSiblingElement("person");
    }
    
    // 5. Create and save an XML document
    cout << "\n=== Test 5: Create a new XML document ===" << endl;
    XMLDocument newDoc;
    
    // Add the declaration
    XMLDeclaration* decl = newDoc.NewDeclaration();
    newDoc.InsertFirstChild(decl);
    
    // Create the root element
    XMLElement* newRoot = newDoc.NewElement("employees");
    newDoc.InsertEndChild(newRoot);
    
    // Add employee 1
    XMLElement* emp1 = newDoc.NewElement("employee");
    emp1->SetAttribute("id", "101");
    emp1->SetAttribute("department", "Engineering");
    
    XMLElement* emp1Name = newDoc.NewElement("name");
    emp1Name->SetText("Alice Johnson");
    emp1->InsertEndChild(emp1Name);
    
    XMLElement* emp1Role = newDoc.NewElement("role");
    emp1Role->SetText("Software Developer");
    emp1->InsertEndChild(emp1Role);
    
    newRoot->InsertEndChild(emp1);
    
    // Add employee 2
    XMLElement* emp2 = newDoc.NewElement("employee");
    emp2->SetAttribute("id", "102");
    
    XMLElement* emp2Name = newDoc.NewElement("name");
    emp2Name->SetText("Bob Williams");
    emp2->InsertEndChild(emp2Name);
    
    newRoot->InsertEndChild(emp2);
    
    // Save to file (file systems may not be supported on RTEMS; this is only a demo)
    cout << "New document created successfully!" << endl;
    
    // 6. Print the new document content
    cout << "\n=== Test 6: Print the new document ===" << endl;
    XMLPrinter printer;
    newDoc.Print(&printer);
    cout << printer.CStr() << endl;
    
    cout << "\nAll tests completed!" << endl;
    exit(0);
}