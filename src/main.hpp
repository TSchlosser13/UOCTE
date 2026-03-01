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

#ifndef MAIN_HPP
#define MAIN_HPP

#include <memory>

#include <QFileDialog>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include "timelineWidget.hpp"
#include "converter.hpp"
#include "patientFilterWidget.hpp"
#include "chooseContoursWidget.hpp"
#include "glSectorWidget.hpp"
#include "io/exportJpeg.hpp"

#include "core/oct_data.hpp"
#include "observer.hpp"

struct oct_subject;
struct oct_scan;

struct dataset
{

  dataset(const QString &path);

  oct_subject m_subject;
  const oct_scan *m_scan;
  observable<std::size_t> m_slice;
  std::unique_ptr<QDialog> m_dialog;
  std::unique_ptr<QWidget> m_widgets[5];

};

class main_window
  : public QMainWindow
{

  Q_OBJECT

public:

  main_window()
    : l(&w), dummy(0), demux(0), key(0)
   {
    menuBar()->addAction("Info", this, SLOT(info()));
    QMenu *m;
    m = menuBar()->addMenu("Main");
    m->addAction("Load", this, SLOT(load_main()));
    m->addAction("Save", this, SLOT(save_main()));
    m->addAction("Save anonymized", this, SLOT(save_anon_main()));
    m->addAction("Export Slices as JPEG", this, SLOT(load_jpeg_exporter_main()));
    m = menuBar()->addMenu("Compare");
    m->addAction("Load", this, SLOT(load_compare()));
    m->addAction("Save", this, SLOT(save_compare()));
    m->addAction("Save anonymized", this, SLOT(save_anon_compare()));
    m->addAction("Export Slices as JPEG", this, SLOT(load_jpeg_exporter_compare()));
    m = menuBar()->addMenu("Timeline");
    m->addAction("Load Timeline", this, SLOT(load_timeline()));
    m->addAction("Change Coloring", this, SLOT(change_coloring()));
    m->addAction("Export for Excel", this, SLOT(export_for_excel()));
    m = menuBar()->addMenu("Converter");
    m->addAction("Convert files and export as JPEG and UOCTML", this, SLOT(convert()));
    m->addAction("Convert anonymized and export as JPEG and UOCTML", this, SLOT(convert_anonymized()));
    setCentralWidget(&w);
    setWindowTitle("Unified OCT Explorer");
    
    connect(&t, SIGNAL(timeout()), this, SLOT(swap()));
    t.setInterval(500);
    t.start();
  }

  QSize sizeHint() const override
  {
    return QSize(800, 600);
  }

  void update(dataset *p);
  void update();

private slots:

  void load_main()
  {
    load(main);
  }

  void load_compare()
  {
    load(compare);
  }

  void save_main()
  {
    save(main, false);
  }

  void save_compare()
  {
    save(compare, false);
  }

  void save_anon_main()
  {
    save(main, true);
  }

  void save_anon_compare()
  {
    save(compare, true);
  }

  void load_jpeg_exporter_main()
  {
      load_jpeg_exporter(main);
  }

  void load_jpeg_exporter_compare()
  {
      load_jpeg_exporter(compare);
  }

  void load_timeline()
  {
      loadTimeline();
  }

  change_coloring()
  {
      changeColoring();
  }

  void convert()
  {
      convertToUoctml(false);
  }

  void convert_anonymized()
  {
      convertToUoctml(true);
  }

  void export_for_excel()
  {
      exportForExcel();
  }
  
  void swap()
  {
    demux = 1 - demux;
  }

  void info()
  {
    QMessageBox::information(this, "Info",
      "Unified OCT Explorer 1.0\n"
      "Copyright (c) 2015 TU Chemnitz\n"
      "NO WARRANTY. NOT CERTIFIED FOR CLINICAL USE.\n"
      "\n"
      "- Load one or two datasets using \"Load Main/Compare\". Supports Topcon OCT, Heidelberg Engineering OCT, Eyetec OCT, and Nidek OCT file formats.\n"
      "- Drag mouse wheel in fundus panel to change active slice.\n"
      "- Drag mouse and mouse wheel to pan and zoom in slice panel.\n"
      "- Drag mouse to rotate in volume rendering panel.\n"
      "- Drag mouse wheel in depth view to change contour.\n"
      "- Use keys \"1\",\"2\",... to hide/show contours."
      ""
      "- Convert several files to list them in the selection for a Timeline View or to export their contour values"
      );
  }

private:

  void keyPressEvent(QKeyEvent *e) override
  {
    key = e->key();
    e->accept();
  }

  void load(std::unique_ptr<dataset> &p);
  void save(const std::unique_ptr<dataset> &p, bool anonymize);
  void load_many(QStringList &paths);
  void load_jpeg_exporter(const std::unique_ptr<dataset> &p);
  void loadTimeline();
  void changeColoring();
  void convertToUoctml(bool anonymized);
  void exportForExcel();

  QTimer t;
  QWidget w;
  QGridLayout l;
  observable<std::size_t> dummy, demux, key;
  std::unique_ptr<dataset> main, compare;

};

class my_button
  : public QPushButton
{

  Q_OBJECT

  main_window &m;
  dataset *p;
  const oct_scan &s;

public:

  my_button(main_window &m, dataset *p, const oct_scan &s, const QString &text)
    : QPushButton(text, p->m_dialog.get()), m(m), p(p), s(s)
  {
    connect(this, SIGNAL(released()), this, SLOT(doit()));
  }

private slots:

  void doit()
  {
    p->m_scan = &s;
    p->m_dialog->close();
    m.update(p);
  }

};

#endif
