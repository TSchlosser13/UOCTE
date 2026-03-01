#ifndef XML_PATIENT_LIST_HPP
#define XML_PATIENT_LIST_HPP

#include <vector>
#include <QDebug>
#include <QString>
#include "io/tinyxml2.h"
#include "core/image.hpp"

class xmlPatientList
{
public:
    //xmlPatientList();
    xmlPatientList(const char *xmlPath);
    ~xmlPatientList();

    struct patient_scan {
        patient_scan():sectorValues(9){}
        std::string id;
        std::string name;
        std::string birth;
        std::string sex; //male or female (M/F)
        std::string scan_date; //human readable (YYYY/MM/DD hh:mm:ss)
        std::string laterality; //left or right eye (L/R)
        std::vector<double> sectorValues;
        std::vector<std::vector<double> > contourValues; //sectorValues for each contour
        double totalVolume;
    };

    void save();
    //keeps patients in ascending order by their ID
    //keeps scans for each patient in ascending order by their date
    //two scans with the same date are not allowed for the same patient
    //scans without a date are inserted at the end
    void addEntry(patient_scan entry);

    QStringList getPatientIDs();
    void getPatientInfo(const char *ID, QString &name, QString &birth, QString &sex);
    std::vector<std::tuple<std::string /*date*/, std::string /*laterality*/,
            std::vector<double> /*sectorvalues*/> > getSectorValues(const char *ID);
    std::vector<std::tuple<std::string /*date*/, std::string /*laterality*/,
            std::vector<std::vector<double> /*sectorvalues of each contour*/> > > getContourValues(const char *ID);


private:

    const char *path;
    tinyxml2::XMLError error;

    tinyxml2::XMLDocument patientList;
    tinyxml2::XMLNode *xmlRoot;
    tinyxml2::XMLElement *xmlElement;

    const char* sectorNames[9];

//METHODS
    //returns true, if ID exists in XML, otherwise false
    //lastVisit points to the patient with the given ID, if he exists in XML
    //, otherwise it points to the patient after whom the ID needs to be inserted for ascending sort
    //, is a nullptr, if it has to be inserted as first element
    bool exists(const char* ID, tinyxml2::XMLNode *&lastVisit);

    //returns true, if scan exists in XML (same date), otherwise false
    //lastVisit MUST point to the first scan of the patient
    //lastVisit points to the scan with the given date, if it exists in XML
    //, otherwise it points to the scan after whom the new scan needs to be inserted for ascending sort
    //, is a nullptr, if it has to be inserted as first element
    bool exists(const patient_scan &entry, tinyxml2::XMLNode* &lastVisit);
    void print(tinyxml2::XMLError error, const char* prevText = "");
};

#endif
