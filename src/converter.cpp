#include "converter.hpp"

namespace Converter
{
    int load(const QString &path, oct_subject** mySubject)
    {
        if (path == "")
            return -1;
        else
        {
            try
            {
                *mySubject = new oct_subject(path.toLocal8Bit().data());
                if ((*mySubject)->scans.empty())
                    qDebug() << "No scans in this file." << endl;
            }
            catch (std::exception &e)
            {
                qDebug() << "Error: " << e.what() << endl;
                return -1;
            }
        }
        return 0;
    }

    int save(const QString &path, oct_subject** mySubject, bool anonymize)
    {
        try
        {
            //uoctml
            if (path != "" && *mySubject != NULL)
                save_uoctml(path.toLocal8Bit().data(), **mySubject, anonymize);
            else
                return -1;
        }
        catch (std::exception &e)
        {
            qDebug() << "Error: " << e.what() << endl;
            return -1;
        }
        return 0;
    }

    void toUoctml(QStringList &inputPaths, QProgressDialog &progress, QPlainTextEdit &output, bool anonymized)
    {
        progress.setValue(0);
        oct_subject* mySubject = NULL;
        unsigned int numLoadable = 0, numSuccess = 0, numTested = 0;

        //create folder "anonymizedData" if it does not exist yet
        //output.appendPlainText("Creating Folder");
        QDir saveFolder(QCoreApplication::applicationDirPath()
                        .append(QDir::separator())
                        .append("anonymizedData"));
        saveFolder.mkdir(".");

        //create XML file for the timeline Visualization
        //output.appendPlainText("Loading XML file");
        xmlPatientList patientList("patient_list.xml");

        //Convert all given OCT data into uoctml format

        int i = 0;
        foreach (QString inputPath, inputPaths)
        {
            numTested++;
            output.appendPlainText(QString("Loading %1 ... ").arg(QFileInfo(inputPath).fileName()));
            output.moveCursor(QTextCursor::End);
            //Check for existing file
            QString newFileName = QFileInfo(inputPath).completeBaseName().append(".uoctml");
            if (saveFolder.exists(newFileName)) {
                output.insertPlainText("exists already");
                output.moveCursor (QTextCursor::End);
            }
            else
            {
                if ( load(inputPath, &mySubject) == -1) {
                    output.insertPlainText("No OCT scans found in this file.");
                    output.moveCursor (QTextCursor::End);
                }
                else
                {
                    output.insertPlainText("done");
                    numLoadable++;

                    //extract Info
                        xmlPatientList::patient_scan patientInfo;
                        patientInfo.id = mySubject->info.at("ID");
                        if (anonymized == false) {
                            if (mySubject->info.find("name") != mySubject->info.end())
                                patientInfo.name = mySubject->info.at("name");
                            if (mySubject->info.find("birth date") != mySubject->info.end())
                                patientInfo.birth = mySubject->info.at("birth date");
                            if (mySubject->info.find("sex") != mySubject->info.end())
                                patientInfo.sex = mySubject->info.at("sex");
                        }

                        for (auto scan = mySubject->scans.begin(); scan != mySubject->scans.end(); ++scan)
                        {
                            //fixation might not be set, but if it is, it has to be "macula"
                            if (scan->second.info["fixation"] == "" || scan->second.info["fixation"] == "macula")
                            {
                                if (scan->second.info.find("scan date") != scan->second.info.end())
                                    patientInfo.scan_date = scan->second.info.at("scan date");
                                if (scan->second.info.find("laterality") != scan->second.info.end())
                                patientInfo.laterality = scan->second.info.at("laterality");

                                calculateSectorValues(scan->second, patientInfo.sectorValues, patientInfo.totalVolume);
                                for (int i = 0; i < 9; ++i)
                                    patientInfo.sectorValues[i] *= 1000;

                                patientInfo.contourValues.clear();
                                calculateContourValues(scan->second, patientInfo.contourValues);
                                for (auto contour = patientInfo.contourValues.begin(); contour != patientInfo.contourValues.end(); ++contour)
                                    for (int i = 0; i < 9; ++i)
                                        (*contour)[i] *= 1000;

                                patientList.addEntry(patientInfo);
                            }
                        }
                    }

                    output.appendPlainText("Converting to uoctml... ");
                    output.moveCursor(QTextCursor::End);
                    if ( save(saveFolder.absoluteFilePath(newFileName), &mySubject, anonymized) == -1) {
                        output.insertPlainText("failed");
                        output.moveCursor (QTextCursor::End);
                    }
                    else {
                        output.insertPlainText("done");
                        output.moveCursor (QTextCursor::End);
                        numSuccess++;
                    }
                    delete mySubject;
                    mySubject = NULL;
            }

            progress.setValue(++i);

            if (progress.wasCanceled()) {
                output.appendPlainText(QString("\nAborting...\n%1 files aborted").arg(inputPaths.size() - numTested));
                break;
            }
        }
        patientList.save();

        output.appendPlainText(QString("\n%1 of %2 loadable files have been converted").arg(numSuccess).arg(numLoadable));
        output.appendPlainText("This Application can now be closed.");
    }

    void calculateSectorValues(const oct_scan &m_scan, std::vector<double> &sectorValues, double &totalVolume, int contour_1, int contour_2)
    {
        //find 2 contours of oct data to calculate their difference
        int test = m_scan.contours.size();
        if ((contour_1 >= test) || (contour_2 >= test)) {
            qDebug() << "Not enough contours in the scan";
            return;
        }

        std::map<std::string, image<float>>::const_iterator m_base;
        std::map<std::string, image<float>>::const_iterator m_other;

        //default
        if (contour_1 < 0 && contour_2 < 0) {
            m_base = m_scan.contours.begin();
            m_other = m_scan.contours.end();
            m_other--; //use last contour

            //ignore NaN as last contour (found in E2E files)
            //use center of image as example for validation
            float test = m_other->second(m_other->second.width()/2,m_other->second.height()/2);
            if (test != test)
                m_other--;

            if (m_base == m_other) {
                qDebug() << "No Contour available";
                return;
            }
        }
        else if (contour_1 >= contour_2) {
            qDebug() << "Contour 2 must be greater than Contour 1";
            return;
        }
        else {
            m_base = m_scan.contours.begin();
            for (int i = 0; i < contour_1; ++i)
                m_base++;
            m_other = m_scan.contours.begin();
            for (int i = 0; i < contour_2; ++i)
                m_other++;
        }

        const image<float> &o1 = m_base->second;
        const image<float> &o2 = m_other->second;

        size_t X = o1.width();
        size_t Y = o1.height();
        if (X != o2.width() || Y != o2.height())
          return;

        //sector values
        double c = 0, no = 0, nr = 0, nl = 0, nu  = 0, fo = 0, fr = 0, fl = 0, fu = 0, t = 0, v = 0;
        double m_aspect = m_scan.size[0] / m_scan.size[2];
        double m_width_in_mm = m_scan.size[0];

        std::unique_ptr<uint8_t []> depth(new uint8_t [X*Y]);

        size_t nc = 0, nno = 0, nnr = 0, nnl = 0, nnu = 0, nfo = 0, nfr = 0, nfl = 0, nfu = 0, n = 0;
        for (size_t y = 0; y != Y; ++y)
        {
          for (size_t x = 0; x != X; ++x)
          {
            double d = std::abs(m_scan.size[1] / m_scan.tomogram.height() * (o2(x, y) - o1(x, y))); // thickness in mm
            double mx = m_scan.size[0] * (2 * x + 1.0 - X) / 2 / X; // x-distance in mm
            double my = m_scan.size[2] * (2 * y + 1.0 - Y) / 2 / Y; // y-distance in mm
            double r2 = mx*mx + my*my; // square of distance

            if (d != d) // skip NaNs
            {
              depth[y*X+x] = 255;
              continue;
            }

            // total volume
            if (r2 <= 9.0)
            {
              t += d;
              ++n;
            }

            depth[y*X+x] = std::min(255.0 / 0.5 * d, 255.0);

            // sector thickness
            if (r2 <= 0.25)
            {
              c += d;
              ++nc;
            }
            else if (r2 <= 2.25)
            {
              if (mx <= my)
              {
                if (mx + my >= 0)
                {
                  nu += d;
                  ++nnu;
                }
                else
                {
                  nl += d;
                  ++nnl;
                }
              }
              else
              {
                if (mx + my >= 0)
                {
                  nr += d;
                  ++nnr;
                }
                else
                {
                  no += d;
                  ++nno;
                }
              }
            }
            else if (r2 <= 9.0)
            {
              if (mx <= my)
              {
                if (mx + my >= 0)
                {
                  fu += d;
                  ++nfu;
                }
                else
                {
                  fl += d;
                  ++nfl;
                }
              }
              else
              {
                if (mx + my >= 0)
                {
                  fr += d;
                  ++nfr;
                }
                else
                {
                  fo += d;
                  ++nfo;
                }
              }
            }
          }
        }

        // average
        c /= nc;
        no /= nno;
        nr /= nnr;
        nl /= nnl;
        nu /= nnu;
        fo /= nfo;
        fr /= nfr;
        fl /= nfl;
        fu /= nfu;
        v = t * m_scan.size[0] * m_scan.size[2] / X / Y;
        t /= n;

        sectorValues = std::vector<double>({c,no,nr,nl,nu,fo,fr,fl,fu});
        totalVolume = v;
    }

    void calculateContourValues(const oct_scan &m_scan, std::vector<std::vector<double> > &contourValues)
    {
        std::map<std::string, image<float>>::const_iterator contour;

        for (auto contour = m_scan.contours.begin(); contour != m_scan.contours.end(); ++contour)
        {
            //ignore NaN
            float test = contour->second(contour->second.width()/2,contour->second.height()/2);
            if (test != test)
                continue;

            const image<float> &o1 = contour->second;

            size_t X = o1.width();
            size_t Y = o1.height();

            double c = 0, no = 0, nr = 0, nl = 0, nu  = 0, fo = 0, fr = 0, fl = 0, fu = 0, t = 0, v = 0;
            double m_aspect = m_scan.size[0] / m_scan.size[2];
            double m_width_in_mm = m_scan.size[0];

            std::unique_ptr<uint8_t []> depth(new uint8_t [X*Y]);

            size_t nc = 0, nno = 0, nnr = 0, nnl = 0, nnu = 0, nfo = 0, nfr = 0, nfl = 0, nfu = 0, n = 0;
            for (size_t y = 0; y != Y; ++y)
            {
              for (size_t x = 0; x != X; ++x)
              {
                double d = std::abs(m_scan.size[1] / m_scan.tomogram.height() * (o1(x, y))); // thickness in mm
                double mx = m_scan.size[0] * (2 * x + 1.0 - X) / 2 / X; // x-distance in mm
                double my = m_scan.size[2] * (2 * y + 1.0 - Y) / 2 / Y; // y-distance in mm
                double r2 = mx*mx + my*my; // square of distance

                if (d != d) // skip NaNs
                {
                  depth[y*X+x] = 255;
                  continue;
                }

                // total volume
                if (r2 <= 9.0)
                {
                  t += d;
                  ++n;
                }

                depth[y*X+x] = std::min(255.0 / 0.5 * d, 255.0);

                // sector thickness
                if (r2 <= 0.25)
                {
                  c += d;
                  ++nc;
                }
                else if (r2 <= 2.25)
                {
                  if (mx <= my)
                  {
                    if (mx + my >= 0)
                    {
                      nu += d;
                      ++nnu;
                    }
                    else
                    {
                      nl += d;
                      ++nnl;
                    }
                  }
                  else
                  {
                    if (mx + my >= 0)
                    {
                      nr += d;
                      ++nnr;
                    }
                    else
                    {
                      no += d;
                      ++nno;
                    }
                  }
                }
                else if (r2 <= 9.0)
                {
                  if (mx <= my)
                  {
                    if (mx + my >= 0)
                    {
                      fu += d;
                      ++nfu;
                    }
                    else
                    {
                      fl += d;
                      ++nfl;
                    }
                  }
                  else
                  {
                    if (mx + my >= 0)
                    {
                      fr += d;
                      ++nfr;
                    }
                    else
                    {
                      fo += d;
                      ++nfo;
                    }
                  }
                }
              }
            }

            // average
            c /= nc;
            no /= nno;
            nr /= nnr;
            nl /= nnl;
            nu /= nnu;
            fo /= nfo;
            fr /= nfr;
            fl /= nfl;
            fu /= nfu;
            v = t * m_scan.size[0] * m_scan.size[2] / X / Y;
            t /= n;

            contourValues.push_back(std::vector<double>({c,no,nr,nl,nu,fo,fr,fl,fu}));
        }
    }
}
