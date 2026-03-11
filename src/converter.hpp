#ifndef UOCTML_CONVERTER_HPP
#define UOCTML_CONVERTER_HPP

#include <algorithm>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressDialog>

#include "io/save_uoctml.hpp"
#include "core/oct_data.hpp"
#include "xmlPatientList.hpp"

namespace Converter{
    extern int load(const QString &path, oct_subject** mySubject);
    extern int save(const QString &path, oct_subject** mySubject, bool anonymize);
    extern void toUoctml(QStringList &inputPaths, QProgressDialog &progress, QPlainTextEdit &output, bool anonymized = false);
    extern void toExcel(QStringList &inputPaths, QProgressDialog &progress, QPlainTextEdit &output, bool anonymized = false);
    //contour_1 starts at 0 (outer makula, nearest to oct-scanner)
    extern void calculateSectorValues(const oct_scan &m_scan, std::vector<double> &sectorValues, double &totalVolume, int contour_1 = -1, int contour_2 = -1);
    extern void calculateContourValues(const oct_scan &m_scan, std::vector<std::vector<double> > &contourValues);
}

#endif //UOCTML_CONVERTER
