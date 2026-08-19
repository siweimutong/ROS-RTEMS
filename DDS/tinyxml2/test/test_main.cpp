// test_tinyxml2_simple.cpp
// 编译: arm-rtems5-g++ test_tinyxml2_simple.cpp -I/path/to/tinyxml2/include -L/path/to/tinyxml2/lib -ltinyxml2 -o test_xml
// 运行: ./test_xml

#include <iostream>
#include "tinyxml2.h"
#include <rtems.h>

using namespace tinyxml2;
using namespace std;

extern "C" rtems_task Init(
  rtems_task_argument ignored
) 
{
    // 1. 测试文档解析
    cout << "=== 测试1: 解析XML字符串 ===" << endl;
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
        cout << "解析失败! 错误代码: " << error << endl;
        exit(1);
    }
    cout << "XML解析成功!" << endl;
    
    // 2. 测试 XMLNode::Value() 函数 (你遇到的错误)
    cout << "\n=== 测试2: 测试 XMLNode::Value() ===" << endl;
    XMLNode* root = doc.FirstChild();
    if (root) {
        const char* rootName = root->Value();
        cout << "根节点名称: " << (rootName ? rootName : "(空)") << endl;
    }
    
    // 3. 测试 XMLElement::GetText()
    cout << "\n=== 测试3: 测试 XMLElement::GetText() ===" << endl;
    XMLElement* firstPerson = doc.FirstChildElement("root")->FirstChildElement("person");
    if (firstPerson) {
        XMLElement* nameElement = firstPerson->FirstChildElement("name");
        if (nameElement) {
            const char* nameText = nameElement->GetText();
            cout << "第一个人物姓名: " << (nameText ? nameText : "(空)") << endl;
        }
    }
    
    // 4. 遍历所有person节点
    cout << "\n=== 测试4: 遍历所有节点 ===" << endl;
    XMLElement* person = doc.FirstChildElement("root")->FirstChildElement("person");
    int count = 0;
    while (person) {
        const char* id = person->Attribute("id");
        const char* name = person->FirstChildElement("name")->GetText();
        const char* age = person->FirstChildElement("age")->GetText();
        
        cout << "人物 " << ++count << ": ID=" << (id ? id : "未知")
             << ", 姓名=" << (name ? name : "未知")
             << ", 年龄=" << (age ? age : "未知") << endl;
        
        person = person->NextSiblingElement("person");
    }
    
    // 5. 创建和保存XML文档
    cout << "\n=== 测试5: 创建新的XML文档 ===" << endl;
    XMLDocument newDoc;
    
    // 添加声明
    XMLDeclaration* decl = newDoc.NewDeclaration();
    newDoc.InsertFirstChild(decl);
    
    // 创建根元素
    XMLElement* newRoot = newDoc.NewElement("employees");
    newDoc.InsertEndChild(newRoot);
    
    // 添加员工1
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
    
    // 添加员工2
    XMLElement* emp2 = newDoc.NewElement("employee");
    emp2->SetAttribute("id", "102");
    
    XMLElement* emp2Name = newDoc.NewElement("name");
    emp2Name->SetText("Bob Williams");
    emp2->InsertEndChild(emp2Name);
    
    newRoot->InsertEndChild(emp2);
    
    // 保存到文件（在RTEMS中可能不支持文件系统，这里只是演示）
    cout << "新文档创建成功!" << endl;
    
    // 6. 打印新文档内容
    cout << "\n=== 测试6: 打印新文档 ===" << endl;
    XMLPrinter printer;
    newDoc.Print(&printer);
    cout << printer.CStr() << endl;
    
    cout << "\n所有测试完成!" << endl;
    exit(0);
}