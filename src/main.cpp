/*
 * Copyright 2015 TU Chemnitz
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "main.hpp"

#include <sstream>

#include <QApplication>
#include <QGLWidget>
#include <QLabel>
#include <QMouseEvent>
#ifdef BUILD_STANDALONE
#include <QtPlugin>
Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
#endif

#include <QDebug>

#include "gl_content.hpp"
#include "io/save_uoctml.hpp"

using namespace std;

namespace
{

string info(const oct_subject &subject, const oct_scan &scan)
{
    vector<string> subject_tags = {"name", "birth date", "sex"};
    vector<string> scan_tags = {"scan date", "laterality"};
    ostringstream o;

    for (auto &s: subject_tags)
    {
        auto i = subject.info.find(s);
        if (i != subject.info.end())
            o << i->first << ": " << i->second << "\n";
    }
    o << "\n";

    for (auto &s: scan_tags)
    {
        auto i = scan.info.find(s);
        if (i != scan.info.end())
            o << i->first << ": " << i->second << "\n";
    }
    o << "\n";

    for (auto &p: subject.info)
        if (find(subject_tags.begin(), subject_tags.end(), p.first) == subject_tags.end())
            o << p.first << ": " << p.second << "\n";

    for (auto &p: scan.info)
        if (find(scan_tags.begin(), scan_tags.end(), p.first) == scan_tags.end())
            o << p.first << ": " << p.second << "\n";

    return o.str();
}

class gl_widget
        : public QGLWidget
{

public:

    gl_widget(std::function<std::unique_ptr<gl_content> (std::function<void ()> &&)> &&make, QWidget *parent = 0)
        : QGLWidget(QGLFormat(), parent), m_timer(), m_make(make)
    {
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
        m_timer.setSingleShot(true);
        m_timer.setInterval(40);
        setMouseTracking(true);
    }

private:

    void post_update()
    {
        if (!m_timer.isActive())
            m_timer.start();
    }

    void initializeGL() override
    {
        m_content = m_make(std::bind(&gl_widget::post_update, this));
    }

    void paintGL() override
    {
        m_content->paint(bind((void (QGLWidget::*)(int, int, const QString &, const QFont &))&QGLWidget::renderText, this, placeholders::_1, placeholders::_2, placeholders::_3, QFont()));
    }

    void resizeGL(int width, int height) override
    {
        m_content->resize(width, height);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        m_content->mouse_press(e->x(), e->y(), e->button());
        e->accept();
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        m_content->mouse_release(e->x(), e->y(), e->button());
        e->accept();
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        m_content->mouse_move(e->x(), e->y(), e->buttons());
        e->accept();
    }

    void wheelEvent(QWheelEvent *e) override
    {
        m_content->mouse_wheel(e->x(), e->y(), e->delta());
        e->accept();
    }

    QSize minimumSizeHint() const override
    {
        return QSize(100, 100);
    }

    QSize sizeHint() const override
    {
        return QSize(400, 400);
    }

    QTimer m_timer;
    std::function<std::unique_ptr<gl_content> (std::function<void ()> &&)> m_make;
    std::unique_ptr<gl_content> m_content;

};

}

unique_ptr<gl_content> make_render_fundus(function<void ()> &&update, const oct_scan &scan, observable<size_t> &slice);
unique_ptr<gl_content> make_render_mip(function<void ()> &&update, const oct_scan &scan, observable<size_t> &key);
unique_ptr<gl_content> make_render_sectors(function<void ()> &&update, const oct_scan &scan);
unique_ptr<gl_content> make_render_slice(function<void ()> &&update, const vector<pair<const oct_scan *, observable<size_t> *>> &scans, observable<size_t> &demux, observable<size_t> &key);

dataset::dataset(const QString &path)
    : m_subject(path.toLocal8Bit().data())
    , m_scan(nullptr)
{
}

void main_window::load(unique_ptr<dataset> &p)
{
    p.reset(0);
    update();

    ostringstream allexts;
    ostringstream alltypes;

    foreach_oct_reader([&](const string &name, const vector<string> &extensions)
    {
        ostringstream exts;

        for (auto &s : extensions)
            exts << " *" << s;

        allexts << exts.str();
        alltypes << ";;" << name << "(" << exts.str() << ")";
    });

    QString path = QFileDialog::getOpenFileName(this, "Choose File", 0, ("OCT data (" + allexts.str() + ")" + alltypes.str()).c_str());
    if (path != "")
    {
        try
        {
            p.reset(new dataset(path));

            if (p->m_subject.scans.empty())
                throw runtime_error("No scans in this file.");

            if (p->m_subject.scans.size() == 1)
            {
                p->m_scan = &p->m_subject.scans.begin()->second;
                update(p.get());
            }
            else
            {
                p->m_dialog.reset(new QDialog(this));
                p->m_dialog->setWindowTitle("Select Scan");
                auto *l = new QVBoxLayout(p->m_dialog.get());
                p->m_dialog->setLayout(l);
                for (auto &e: p->m_subject.scans)
                {
                    string s = e.first;
                    auto d = e.second.info.find("scan date");
                    if (d != e.second.info.end())
                        s += " (" + d->second + ")";
                    l->addWidget(new my_button(*this, p.get(), e.second, QString::fromUtf8(s.c_str())));
                }
                p->m_dialog->show();
            }
        }
        catch (exception &e)
        {
            QMessageBox::critical(this, "Error", e.what());
        }
    }
}

void main_window::load_many(QStringList &paths)
{
    ostringstream allexts;
    ostringstream alltypes;

    foreach_oct_reader([&](const string &name, const vector<string> &extensions)
    {
        ostringstream exts;

        for (auto &s : extensions)
            exts << " *" << s;

        allexts << exts.str();
        alltypes << ";;" << name << "(" << exts.str() << ")";
    });

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setNameFilter(("OCT data (" + allexts.str() + ")" + alltypes.str()).c_str());

    if (dialog.exec())
        paths = dialog.selectedFiles();
}


void main_window::load_jpeg_exporter(const std::unique_ptr<dataset> &p)
{
    if (p == nullptr)
    {
        QMessageBox noDatasetLoaded;
        noDatasetLoaded.setText("Please load an OCT file before proceeding.");
        noDatasetLoaded.addButton("OK", QMessageBox::AcceptRole);
        noDatasetLoaded.exec();
        return;
    }
    if (p->m_scan == nullptr)
    {
        QMessageBox noScanSelected;
        noScanSelected.setText("Please select one of the scans found in the given OCT file before proceeding.");
        noScanSelected.addButton("OK", QMessageBox::AcceptRole);
        noScanSelected.exec();
        return;
    }

    //choose contours here
    ChooseContoursWidget chooseContours(p->m_scan->contours.size());
    if (chooseContours.exec() == QDialog::Accepted)
    {
        //create folder "Export" if it does not exist yet
        QDir saveFolder(QDir::currentPath()
                        .append(QDir::separator())
                        .append("Export"));
        saveFolder.mkdir(".");

        //prepare filename
        string filename = saveFolder.absolutePath().toLocal8Bit().constData();
        filename.append("/");
        filename.append(p->m_subject.info.at("ID")).append("_");
        if (p->m_scan->info.find("scan date") != p->m_scan->info.end()) {
            //make date format compatible to file names
            string date = p->m_scan->info.at("scan date");
            replace( date.begin(), date.end(), '/', '-');
            replace( date.begin(), date.end(), ' ', '_');
            date.replace( date.find_first_of(':'), 1, "h");
            date.replace( date.find_first_of(':'), 1, "m");
            date.append("s");
            filename.append(date).append("_");
        }
        if (p->m_scan->info.find("laterality") != p->m_scan->info.end())
            filename.append(p->m_scan->info.at("laterality")).append("_");

        //message staying as long as files are getting exported
        QMessageBox progressInfo;
        progressInfo.setText("Exporting to JPEG ...");
        progressInfo.addButton(QMessageBox::Ok);
        progressInfo.button(QMessageBox::Ok)->hide();
        progressInfo.show();

        //export images in own thread
        QEventLoop loop;
        loop.processEvents();
        exportSlicesAsJpeg(p->m_scan, filename, chooseContours.getContourList(), chooseContours.getContourColor());
        loop.exit();

        //progressInfo.button(QMessageBox::Ok)->show();
    }
}

void main_window::loadTimeline()
{
    xmlPatientList *patientList = new xmlPatientList("patient_list.xml");
    if (patientList->getPatientIDs().empty())
    {
        QMessageBox noPatientFiles;
        noPatientFiles.addButton("OK", QMessageBox::AcceptRole);
        noPatientFiles.setText("No patient data was found.\nPlease use the built in converter for your OCT files before proceeding.");
        noPatientFiles.exec();
    }
    else
    {
        QString patientID;
        //create dialog to choose a patient ID
        PatientFilterWidget *dialog = new PatientFilterWidget(patientList);
        if(dialog->exec() == QDialog::Accepted)
        {
            patientID = dialog->getSelectedItem();
            if (patientID == "")
                return;

            TimelineWidget *tlw = new TimelineWidget(this);
            QString name, birth, sex;
            patientList->getPatientInfo(patientID.toLatin1().data(), name, birth, sex);
            tlw->setPatientData(name, birth, sex, patientList->getSectorValues(patientID.toLatin1().data()));
            tlw->drawTimeline();
            tlw->show();
            //tlw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
    }
}

void main_window::changeColoring()
{
    QMessageBox notImplemented;
    notImplemented.addButton("OK", QMessageBox::AcceptRole);
    notImplemented.setText("Not implemented yet.");
    notImplemented.exec();
}

void main_window::exportForExcel()
{
    xmlPatientList *patientList = new xmlPatientList("patient_list.xml");
    if (patientList->getPatientIDs().empty())
    {
        QMessageBox noPatientFiles;
        noPatientFiles.addButton("OK", QMessageBox::AcceptRole);
        noPatientFiles.setText("No patient data was found.\nPlease use the built in converter for your OCT files before proceeding.");
        noPatientFiles.exec();
    }
    else
    {
        QString patientID;
        //create dialog to choose a patient ID
        PatientFilterWidget *dialog = new PatientFilterWidget(patientList);
        if(dialog->exec() == QDialog::Accepted)
        {
            patientID = dialog->getSelectedItem();
            if (patientID == "")
                return;

            QString name, birth, sex;
            patientList->getPatientInfo(patientID.toLatin1().data(), name, birth, sex);
            auto values = patientList->getContourValues(patientID.toLatin1().data());

            //create CSV file
            //CSV can be converted to excel tables - commas seperate columns, newlines seperate rows
            QString filename (QString("%1%2").arg(patientID).arg(".csv"));
            QFile file(filename);
            if(file.exists())
                if (file.remove() == false) {
                    qDebug() << "Could not overwrite existing file.";
                    return;
                }
            if (file.open(QIODevice::WriteOnly) == false) {
                qDebug() << "Could not write to the file.";
                return;
            }

            std::ostringstream filestream, leftLatStream, rightLatStream, unknownLatStream;

            //prepare output of contour values
            std::ostringstream *latStream;
            for (auto scan = values.begin(); scan != values.end(); ++scan)
            {
                if (std::get<1>(*scan) == "L")
                    latStream = &leftLatStream;
                else if (std::get<1>(*scan) == "R")
                    latStream = &rightLatStream;
                else
                    latStream = &unknownLatStream;

                int numContour = 0;
                for (auto contour = std::get<2>(*scan).begin(); contour != std::get<2>(*scan).end(); ++contour)
                {
                    *latStream << std::get<0>(*scan) << "," << numContour;
                    for (auto sectorValue = contour->begin(); sectorValue != contour->end(); ++sectorValue)
                        *latStream << "," << *sectorValue;
                    *latStream << "\n";
                    numContour++;
                }
                *latStream << "\n";
            }

            //write patient information
            filestream << "ID," << patientID.toStdString();
            filestream << "\nName," << name.toStdString();
            filestream << "\nBirth date," << birth.toStdString();
            filestream << "\nSex," << sex.toStdString();
            filestream << "\n";

            //write values of left eye to csv
            if (!leftLatStream.str().empty()) {
                filestream << "\nLeft Eye\n";
                filestream << "Scan date,Contour,outer top,outer left,inner top,inner left,center,inner right,inner bottom,outer right,outer bottom\n";
                filestream << leftLatStream.str().c_str();
            }


            //write values of right eye to csv
            if (!rightLatStream.str().empty()) {
                filestream << "\nRight Eye\n";
                filestream << "Scan date,Contour,outer top,outer left,inner top,inner left,center,inner right,inner bottom,outer right,outer bottom\n";
                filestream << rightLatStream.str().c_str();
            }

            //write values of unknown laterality
            if (!unknownLatStream.str().empty()) {
                filestream << "\nUnknown Laterality\n";
                filestream << "Scan date,Contour,outer top,outer left,inner top,inner left,center,inner right,inner bottom,outer right,outer bottom\n";
                filestream << unknownLatStream.str().c_str();
            }

            file.write(filestream.str().c_str());
            file.close();
        }
    }
}

void main_window::convertToUoctml(bool anonymized)
{
    //load OCT files
    QStringList inputPaths;
    load_many(inputPaths);

    //create output log
    QWidget *dialog = new QWidget();
    dialog->setWindowTitle("Converting");

    QProgressDialog progress (QString("%1 file(s) selected").arg(inputPaths.size()), "Cancel", 0, inputPaths.size());
    progress.setWindowModality(Qt::WindowModal);

    //QProgressBar *progress = new QProgressBar(dialog);

    QPlainTextEdit *output = new QPlainTextEdit();
    output->setReadOnly(true);

    //QPushButton* rejectButton = new QPushButton("cancel");
    QPushButton *acceptButton = new QPushButton("Finish");

    QVBoxLayout mainLayout;
        mainLayout.addWidget(output);
        mainLayout.addWidget(&progress);
        mainLayout.addWidget(acceptButton);
    dialog->setLayout(&mainLayout);

    acceptButton->hide();

    //connect(progress, SIGNAL(), &acceptButton, SLOT(show()));
    connect(acceptButton, SIGNAL(released()), dialog, SLOT(close()));

    dialog->show();

    QEventLoop loop;
    loop.processEvents();
    Converter::toUoctml(inputPaths, progress, *output, anonymized);
    loop.exit();

    acceptButton->show();
    dialog->raise();
}

void main_window::update(dataset *p)
{
    const oct_scan &s = *p->m_scan;
    p->m_slice = s.tomogram.depth() / 2; // initially select middle slice
    p->m_widgets[0].reset(new QLabel(QString::fromUtf8(::info(p->m_subject, s).c_str())));
    p->m_widgets[1].reset(new gl_widget(bind(&make_render_fundus, placeholders::_1, cref(s), ref(p->m_slice))));
    p->m_widgets[2].reset(new gl_widget(bind(&make_render_mip, placeholders::_1, cref(s), ref(key))));
    p->m_widgets[3].reset(new gl_widget(bind(&make_render_sectors, placeholders::_1, cref(s))));
    /*glSectorWidget *sectorView = new glSectorWidget();
    //sectorView->setFixedSize(sectorView->getWidth(), sectorView->getHeight());
    sectorView->setAttribute(Qt::WA_DontCreateNativeAncestors);
    p->m_widgets[3].reset(sectorView);
    */

    update();
}

void main_window::update()
{
    if (main && compare)
    {
        main->m_widgets[4].reset(new gl_widget(bind(&make_render_slice, placeholders::_1, vector<pair<const oct_scan *, observable<size_t> *>>{make_pair(main->m_scan, &main->m_slice)}, ref(dummy), ref(key))));
        compare->m_widgets[4].reset(new gl_widget(bind(&make_render_slice, placeholders::_1, vector<pair<const oct_scan *, observable<size_t> *>>{make_pair(main->m_scan, &main->m_slice), make_pair(compare->m_scan, &compare->m_slice)}, ref(demux), ref(key))));

        l.addWidget(main->m_widgets[0].get(), 0, 0);
        l.addWidget(main->m_widgets[1].get(), 1, 0);
        l.addWidget(main->m_widgets[2].get(), 1, 1);
        l.addWidget(main->m_widgets[3].get(), 0, 1);
        l.addWidget(main->m_widgets[4].get(), 2, 0, 1, 2);
        l.addWidget(compare->m_widgets[0].get(), 0, 3);
        l.addWidget(compare->m_widgets[1].get(), 1, 3);
        l.addWidget(compare->m_widgets[2].get(), 1, 2);
        l.addWidget(compare->m_widgets[3].get(), 0, 2);
        l.addWidget(compare->m_widgets[4].get(), 2, 2, 1, 2);
    }
    else if (main)
    {
        main->m_widgets[4].reset(new gl_widget(bind(&make_render_slice, placeholders::_1, vector<pair<const oct_scan *, observable<size_t> *>>{make_pair(main->m_scan, &main->m_slice)}, ref(dummy), ref(key))));
        l.addWidget(main->m_widgets[0].get(), 0, 0);
        l.addWidget(main->m_widgets[1].get(), 1, 0);
        l.addWidget(main->m_widgets[2].get(), 0, 1);
        l.addWidget(main->m_widgets[3].get(), 0, 2);
        l.addWidget(main->m_widgets[4].get(), 1, 1, 1, 2);
    }
    else if (compare)
    {
        compare->m_widgets[4].reset(new gl_widget(bind(&make_render_slice, placeholders::_1, vector<pair<const oct_scan *, observable<size_t> *>>{make_pair(compare->m_scan, &compare->m_slice)}, ref(dummy), ref(key))));
        l.addWidget(compare->m_widgets[0].get(), 0, 5);
        l.addWidget(compare->m_widgets[1].get(), 1, 5);
        l.addWidget(compare->m_widgets[2].get(), 0, 4);
        l.addWidget(compare->m_widgets[3].get(), 0, 3);
        l.addWidget(compare->m_widgets[4].get(), 1, 3, 1, 2);
    }
}

void main_window::save(const unique_ptr<dataset> &p, bool anonymize)
{
    try
    {
        if (!p)
            throw runtime_error("dataset not loaded");

        QString path = QFileDialog::getSaveFileName(this, "Save File", 0, "UOCTML (*.uoctml)");

        if (path != "")
            save_uoctml(path.toLocal8Bit().data(), p->m_subject, anonymize);
    }
    catch (exception &e)
    {
        QMessageBox::critical(this, "Error", e.what());
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    main_window w;
    w.show();

    return app.exec();
}
