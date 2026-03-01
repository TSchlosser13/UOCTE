#include "xmlPatientList.hpp"

using namespace tinyxml2;

xmlPatientList::xmlPatientList(const char *path)
    : sectorNames {"c", "no", "nr", "nl", "nu", "fo", "fr", "fl", "fu"}
/* if path does not exist, it will be created
 * path will be an XML file
 * ATTENTION: path will be overwritten, no matter what file it is
 */
{
    this->path = path;
    error = patientList.LoadFile(path);
    //if file does not exist
    if (error != XML_SUCCESS) {
        if(error == XML_ERROR_FILE_NOT_FOUND ||
                error == XML_ERROR_EMPTY_DOCUMENT) {
            //create default file
            patientList.NewDeclaration();
            xmlRoot = patientList.NewElement("patientList");
            patientList.InsertFirstChild(xmlRoot);
        }
        else
            print(error, "Load ");
    }
    xmlRoot = patientList.RootElement();
    if (xmlRoot == nullptr)
        print(tinyxml2::XML_ERROR_FILE_READ_ERROR, "find root ");

    //insert date
    /*xmlElement = patientList.NewElement("Date");
    xmlElement->SetAttribute("day", 26);
    xmlElement->SetAttribute("month", "April");
    xmlElement->SetAttribute("year", 2014);
    xmlElement->SetAttribute("dateFormat", "26/04/2014");
    xmlRoot->InsertEndChild(xmlElement);
    */
    //for (const auto & item : vecList)

    //save file
    /*eResult = timelineXML.SaveFile("timeline.xml");
    if (eResult != tinyxml2::XML_SUCCESS)
        qout << "Save Error: " << eResult << endl;
    */
}

xmlPatientList::~xmlPatientList()
{
}

void xmlPatientList::save()
{
    error = patientList.SaveFile(path);
    if (error != tinyxml2::XML_SUCCESS)
        if(error == XML_ERROR_FILE_NOT_FOUND ||
                error == XML_ERROR_EMPTY_DOCUMENT)
            ;//file did not exist and has been created
        else
            print(error, "Save ");
}

QStringList xmlPatientList::getPatientIDs()
{
    QStringList patientIDs;

    //find first patient
    tinyxml2::XMLNode* xmlCurrentNode = xmlRoot->FirstChild();

    //go through all patients
    while(xmlCurrentNode != nullptr)
    {
        patientIDs.push_back(xmlCurrentNode->ToElement()->Attribute("ID"));
        xmlCurrentNode = xmlCurrentNode->NextSibling();
    }

    return patientIDs;
}

void xmlPatientList::getPatientInfo(const char *ID, QString &name, QString &birth, QString &sex)
{
    tinyxml2::XMLNode* xmlCurrentNode;
    if (!exists(ID, xmlCurrentNode))
        return;
    if (xmlCurrentNode->FirstChildElement("name") != nullptr)
        name = QString(xmlCurrentNode->FirstChildElement("name")->GetText());
    if (xmlCurrentNode->FirstChildElement("birth") != nullptr)
        birth = xmlCurrentNode->FirstChildElement("birth")->GetText();
    if (xmlCurrentNode->FirstChildElement("sex") != nullptr)
        sex = xmlCurrentNode->FirstChildElement("sex")->GetText();
}

bool xmlPatientList::exists(const char* ID, tinyxml2::XMLNode* &lastVisit)
{
    lastVisit = nullptr;

    //find first patient
    tinyxml2::XMLNode* xmlCurrentNode = xmlRoot->FirstChild();
    tinyxml2::XMLNode* xmlPrevNode = nullptr;

    //go through all patients and test, if their IDs match the given ID
    while (xmlCurrentNode != nullptr)
    {
        int val = strcmp(xmlCurrentNode->ToElement()->Attribute("ID"), ID);
        if (val == 0) { //ID was found
            //qDebug() << QString("%1 is equal to %2").arg(ID).arg(xmlCurrentNode->ToElement()->Attribute("ID"));
            lastVisit = xmlCurrentNode;
            return true;
        }
        else if (val > 0) { //ID does not exist but has to be inserted before the current node
            //qDebug() << QString("%1 is smaller than %2").arg(ID).arg(xmlCurrentNode->ToElement()->Attribute("ID"));
            lastVisit = xmlPrevNode;
            return false;
        }
        else { //val < 0
            xmlPrevNode = xmlCurrentNode;
            xmlCurrentNode = xmlCurrentNode->NextSibling();
        }
    }
    //on exit the ID is not in the XML and has to be inserted at the end of the file to keep ascending sort
    lastVisit = xmlPrevNode;
    return false;
}

bool xmlPatientList::exists(const patient_scan &entry, tinyxml2::XMLNode* &lastVisit)
{
    //find first patient
    tinyxml2::XMLNode* currentScan = lastVisit;
    tinyxml2::XMLNode* prevScan = nullptr;

    //if entries date is unknown
    if (entry.scan_date.empty() || (strcmp(entry.scan_date.c_str(), "unknown") == 0)) {
        lastVisit = nullptr;
        return false;
    }

    //go through all scans and test, if their dates match the given scans date
    while (currentScan != nullptr)
    {
        int val = strcmp(currentScan->FirstChildElement("date")->GetText(), entry.scan_date.c_str());
        if (val == 0) { //date was found
            lastVisit = currentScan;
            return true;
        }
        else if (val > 0) { //ID does not exist but has to be inserted before the current node
            lastVisit = prevScan;
            return false;
        }
        else { //val < 0
            prevScan = currentScan;
            currentScan = currentScan->NextSiblingElement("scan");
        }
    }
    //on exit the ID is not in the XML and has to be inserted at the end of the file to keep ascending sort
    return false;
}

void xmlPatientList::addEntry(xmlPatientList::patient_scan entry)
{
    tinyxml2::XMLNode* xmlCurrentNode;

    //if patient does not exist in XML, create him
    if (!exists(entry.id.c_str(), xmlCurrentNode))
    {
        xmlElement = patientList.NewElement("patient");
        xmlElement->SetAttribute("ID", entry.id.c_str());
        if (xmlCurrentNode == nullptr)
            xmlRoot->InsertFirstChild(xmlElement);
        else {
            xmlRoot->InsertAfterChild(xmlCurrentNode, xmlElement);
        }
        xmlCurrentNode = xmlElement;
    }

    //insert meta data, if it does not exist
    if (xmlCurrentNode->FirstChildElement("name") == nullptr) {
        if (!entry.name.empty()) {
            xmlElement = patientList.NewElement("name");
            xmlElement->SetText(entry.name.c_str());
            xmlCurrentNode->InsertFirstChild(xmlElement);
        }
    }

    if (xmlCurrentNode->FirstChildElement("birth") == nullptr) {
        if (!entry.birth.empty()) {
            xmlElement = patientList.NewElement("birth");
            xmlElement->SetText(entry.birth.c_str());
            xmlCurrentNode->InsertFirstChild(xmlElement);
        }
    }

    if (xmlCurrentNode->FirstChildElement("sex") == nullptr) {
        if (!entry.sex.empty()) {
            xmlElement = patientList.NewElement("sex");
            xmlElement->SetText(entry.sex.c_str());
            xmlCurrentNode->InsertFirstChild(xmlElement);
        }
    }

    //insert new scan
    tinyxml2::XMLNode* findScan = xmlCurrentNode->FirstChildElement("scan");
    if(!exists(entry, findScan))
    {
        xmlElement = patientList.NewElement("scan");
        //xmlElement->SetAttribute("date", entry.scan_date.c_str());
        if (findScan == nullptr)
            xmlCurrentNode->InsertFirstChild(xmlElement);
        else
            xmlCurrentNode->InsertAfterChild(findScan, xmlElement);

        xmlCurrentNode = xmlElement;

        if(!entry.scan_date.empty()) {
            xmlElement = patientList.NewElement("date");
            xmlElement->SetText(entry.scan_date.c_str());
            xmlCurrentNode->InsertEndChild(xmlElement);
        }

        xmlElement = patientList.NewElement("sectorValues");
        for(int i = 0; i < 9; ++i) {
            //round to natural numbers
            xmlElement->SetAttribute(sectorNames[i], int(entry.sectorValues[i]+0.5)); // +0.5 to round mathematically
        }
        xmlCurrentNode->InsertEndChild(xmlElement);
        xmlCurrentNode = xmlElement;
        for (int contour = 0; contour < entry.contourValues.size(); ++contour) {
            xmlElement = patientList.NewElement("contour");
            xmlElement->SetAttribute("id", contour);
            for(int i = 0; i < 9; ++i) {
                //round to natural numbers
                xmlElement->SetAttribute(sectorNames[i], int(entry.contourValues[contour][i]+0.5));
            }
            xmlCurrentNode->InsertEndChild(xmlElement);
        }
        xmlCurrentNode = xmlCurrentNode->Parent();

        xmlElement = patientList.NewElement("totalVolume");
        xmlElement->SetText(entry.totalVolume);
        xmlCurrentNode->InsertEndChild(xmlElement);

        if (!entry.laterality.empty()) {
            xmlElement = patientList.NewElement("laterality");
            xmlElement->SetText(entry.laterality.c_str());
            xmlCurrentNode->InsertEndChild(xmlElement);
        }

        xmlCurrentNode = xmlElement;
    }
}

std::vector<std::tuple<std::string /*date*/, std::string /*laterality*/,
            std::vector<double> /*sectorValues*/> > xmlPatientList::getSectorValues(const char* ID)
{
    std::vector<std::tuple<std::string,std::string,std::vector<double> > > scans;
    //find first patient
    tinyxml2::XMLNode* xmlCurrentNode = xmlRoot->FirstChild();

    std::string date;
    std::string laterality;

    //go through all patients and test, if their IDs match the given ID
    while (xmlCurrentNode != nullptr)
    {
        int val = strcmp(xmlCurrentNode->ToElement()->Attribute("ID"), ID);
        if (val == 0) { //ID was found
            tinyxml2::XMLNode* currentScan = xmlCurrentNode->FirstChildElement("scan");
            //go through all scans of this patient
            int i = 1;
            while (currentScan != nullptr)
            {
                date.clear();
                laterality.clear();
                if (currentScan->FirstChildElement("date") != nullptr)
                    date = std::string(currentScan->FirstChildElement("date")->GetText());
                else
                    date = std::string("unknown");
                if (currentScan->FirstChildElement("laterality") != nullptr)
                    laterality = std::string(currentScan->FirstChildElement("laterality")->GetText());
                else
                    laterality = std::string("unknown");
                i++;
                std::vector<double> sectorValues;
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("fu"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("fl"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("nu"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("nl"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("c"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("nr"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("no"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("fr"));
                sectorValues.push_back(currentScan->FirstChildElement("sectorValues")->DoubleAttribute("fo"));

                scans.push_back(std::tuple<std::string,std::string,std::vector<double> > (date, laterality, sectorValues));
                currentScan = currentScan->NextSiblingElement("scan");
            }
            return scans;
        }
        else if (val > 0) { //ID does not exist, because list is ordered ascending
            return scans;
        }
        else { //val < 0
            xmlCurrentNode = xmlCurrentNode->NextSibling();
        }
    }
    //ID does not exist
    return scans;
}

std::vector<std::tuple<std::string /*date*/, std::string /*laterality*/,
        std::vector<std::vector<double> /*sectorvalues of each contour*/> > > xmlPatientList::getContourValues(const char *ID)
{
    std::vector<std::tuple<std::string, std::string, std::vector<std::vector<double> > > > scans;
    //find first patient
    tinyxml2::XMLNode* xmlCurrentNode = xmlRoot->FirstChild();

    std::string date;
    std::string laterality;

    //go through all patients and test, if their IDs match the given ID
    while (xmlCurrentNode != nullptr)
    {
        int val = strcmp(xmlCurrentNode->ToElement()->Attribute("ID"), ID);
        if (val == 0) { //ID was found
            tinyxml2::XMLNode* currentScan = xmlCurrentNode->FirstChildElement("scan");
            //go through all scans of this patient
            int i = 1;
            while (currentScan != nullptr)
            {
                date.clear();
                laterality.clear();
                if (currentScan->FirstChildElement("date") != nullptr)
                    date = std::string(currentScan->FirstChildElement("date")->GetText());
                else
                    date = std::string("unknown");
                if (currentScan->FirstChildElement("laterality") != nullptr)
                    laterality = std::string(currentScan->FirstChildElement("laterality")->GetText());
                else
                    laterality = std::string("unknown");
                i++;

                tinyxml2::XMLNode* currentContour = currentScan->FirstChildElement("sectorValues")->FirstChildElement("contour");
                std::vector<std::vector<double> > contourValues;
                while (currentContour != nullptr)
                {
                    std::vector<double> sectorValues;
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("fu"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("fl"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("nu"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("nl"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("c"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("nr"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("no"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("fr"));
                    sectorValues.push_back(currentContour->ToElement()->DoubleAttribute("fo"));

                    contourValues.push_back(sectorValues);
                    currentContour = currentContour->NextSiblingElement("contour");
                }

                scans.push_back(std::tuple<std::string,std::string,std::vector<std::vector<double> > > (date, laterality, contourValues));
                currentScan = currentScan->NextSiblingElement("scan");
            }
            return scans;
        }
        else if (val > 0) { //ID does not exist, because list is ordered ascending
            return scans;
        }
        else { //val < 0
            xmlCurrentNode = xmlCurrentNode->NextSibling();
        }
    }
    //ID does not exist
    return scans;
}

void xmlPatientList::print(XMLError error, const char* prevText)
{
    QTextStream qout(stdout);
    qout << prevText << "Error: " << error << endl;
}
