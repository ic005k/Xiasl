#include "mainwindow.h"

#include <QTabBar>
#include <QTabWidget>

#include "MyTabBar.h"
#include "MyTabPage.h"
#include "MyTabPopup.h"
#include "filesystemwatcher.h"
#include "mytabwidget.h"
#include "ui_mainwindow.h"
#ifdef __APPLE__
#include "OSXHideTitleBar.h"
#endif
#include "methods.h"

QString CurVersion = "1.1.67";
QString fileName, curFile, dragFileName, findStr, findPath, search_string,
    curFindFile;

bool loading = false;
bool thread_end = true;
bool break_run = false;
bool show_s = true;
bool show_d = true;
bool show_m = true;
bool show_n = false;
bool textEditScroll = false;
bool miniEditWheel = false;
bool ReLoad = false;
bool zh_cn = false;
bool isIncludeSubDir, isCaseSensitive;
bool isFinishFind = true;
bool isBreakFind = false;
QList<int> m_searchTextPosList;
QStringList files;

int s_count = 0;
int m_count = 0;
int d_count = 0;
int n_count = 0;
int vs, hs, red, rowDrag, colDrag;
long curFindPos;
long totalPos;

QsciScintilla *textEditBack, *textEditSerach;
QsciScintilla* miniDlgEdit;
miniDialog* miniDlg;

// QVector<QsciScintilla*> textEditList;
QVector<QString> openFileList;

QList<QTreeWidgetItem*> twitems;
QList<QTreeWidgetItem*> tw_scope;
QList<QTreeWidgetItem*> tw_device;
QList<QTreeWidgetItem*> tw_method;
QList<QTreeWidgetItem*> tw_name;
QList<QTreeWidgetItem*> tw_list;
QList<QTreeWidgetItem*> tw_SearchResults;
QTreeWidget* treeWidgetBak;

QVector<QString> filelist;
QWidgetList wdlist;
QsciLexer *myTextLexer, *miniLexer;

extern MainWindow* mw_one;

thread_one::thread_one(QObject* parent) : QThread(parent) {}
SearchThread::SearchThread(QObject* parent) : QThread{parent} {}

MiniEditor::MiniEditor(QWidget* parent) : QsciScintilla(parent) {
  setContextMenuPolicy(Qt::NoContextMenu);
  connect(this, &QsciScintilla::cursorPositionChanged, this,
          &MiniEditor::miniEdit_cursorPositionChanged);
}

MaxEditor::MaxEditor(QWidget* parent) : QsciScintilla(parent) {}

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
  lineNumberArea = new LineNumberArea(this);

  connect(this, &CodeEditor::blockCountChanged, this,
          &CodeEditor::updateLineNumberAreaWidth);
  connect(this, &CodeEditor::updateRequest, this,
          &CodeEditor::updateLineNumberArea);
  connect(this, &CodeEditor::cursorPositionChanged, this,
          &CodeEditor::highlightCurrentLine);

  updateLineNumberAreaWidth(0);
  highlightCurrentLine();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  loading = true;

  loadLocal();

#ifdef Q_OS_WIN32
  regACPI_win();
  ui->actionKextstat->setEnabled(false);
  win = true;
#endif

#ifdef Q_OS_LINUX
  ui->actionKextstat->setEnabled(false);
  ui->actionGenerate->setEnabled(false);
  linuxOS = true;
#endif

#ifdef Q_OS_MAC
  ui->actionGenerate->setEnabled(true);
  mac = true;

#if (QT_VERSION <= QT_VERSION_CHECK(5, 15, 0))
  osx1012 = true;
  mac = false;
#endif

#endif

  init_Widget();

  init_statusBar();

  init_menu();

  init_recentFiles();

  init_info_edit();

  init_treeWidget();

  init_UIStyle();

  init_UI_Layout();

  init_filesystem();

  init_Tool_UI();

  loadTabFiles();

  loadFindString();

  readINIProxy();

  init_ScrollBox();

  loading = false;
}

MainWindow::~MainWindow() {
  delete ui;

  mythread->quit();
  mythread->wait();

  mySearchThread->quit();
  mySearchThread->wait();
}

void MainWindow::init_Widget() {
  strIniFile =
      QDir::homePath() + "/.config/" + strAppName + "/" + strAppName + ".ini";

  installEventFilter(this);

  listMd5.clear();
  ver = "Xiasl V" + CurVersion + "        ";
  setWindowTitle(ver);
  ver = "";

  //获取背景色
  QPalette pal = ui->treeWidget->palette();
  QBrush brush = pal.window();
  red = brush.color().red();

  QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

  mythread = new thread_one();
  connect(mythread, &thread_one::over, this, &MainWindow::dealover);
  mySearchThread = new SearchThread();
  connect(mySearchThread, &SearchThread::isDone, this, &MainWindow::dealDone);

  dlg = new dlgDecompile(this);
  dlgAutoUpdate = new AutoUpdateDialog(this);
  dlgset = new dlgPreferences(this);

  miniDlg = new miniDialog(this);
  miniDlg->close();

  miniEdit = new MiniEditor(this);

  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(timer_linkage()));
  tmeShowFindProgress = new QTimer(this);
  connect(tmeShowFindProgress, SIGNAL(timeout()), this,
          SLOT(on_ShowFindProgress()));

  manager = new QNetworkAccessManager(this);
  connect(manager, SIGNAL(finished(QNetworkReply*)), this,
          SLOT(replyFinished(QNetworkReply*)));
  blAutoCheckUpdate = true;
  CheckUpdate();
}

void MainWindow::init_UI_Layout() {
  // 分割窗口

  QSettings Reg(strIniFile, QSettings::IniFormat);
  ui->centralwidget->layout()->setContentsMargins(2, 2, 2, 2);
  ui->centralwidget->layout()->setSpacing(1);
  QSplitter* splitterH = new QSplitter(Qt::Horizontal, this);
  ui->vLayout->addWidget(ui->frameTip);
  ui->vLayout->addWidget(ui->tabWidget_textEdit);

  splitterH->addWidget(ui->tabWidget_misc);
  splitterH->addWidget(ui->frame);

  ui->centralwidget->layout()->addWidget(splitterH);
  int w0 = Reg.value("w0", 200).toInt();
  int w1 = Reg.value("w1", 450).toInt();
  if (w0 < 150) w0 = 150;
  if (w1 < 150) w1 = 150;
  QList<int> list;
  list.append(w0);
  list.append(w1);
  splitterH->setSizes(list);
  connect(splitterH, &QSplitter::splitterMoved, [=]() {});

  QSplitter* splitterV = new QSplitter(Qt::Vertical, this);
  ui->frameInfo->layout()->addWidget(ui->listWidget);
  ui->frameInfo->layout()->addWidget(ui->tabWidget);
  splitterV->addWidget(splitterH);
  splitterV->addWidget(ui->frameInfo);
  ui->centralwidget->layout()->addWidget(splitterV);
  int h0 = Reg.value("h0", 500).toInt();
  int h1 = Reg.value("h1", 100).toInt();
  if (h0 < 150) h0 = 150;
  if (h1 < 100) h1 = 100;
  list.clear();
  list.append(h0);
  list.append(h1);
  splitterV->setSizes(list);
  connect(splitterV, &QSplitter::splitterMoved, [=]() {

  });

  ui->centralwidget->layout()->addWidget(ui->frameBook);

  ui->fBox->setMouseTracking(true);
  ui->dockMiniEdit->layout()->addWidget(miniEdit);
  ui->dockMiniEdit->layout()->addWidget(ui->fBox);

  //设置鼠标追踪
  ui->centralwidget->setMouseTracking(true);
  this->setMouseTracking(true);
  ui->frameInfo->setMouseTracking(true);
  ui->statusbar->setMouseTracking(true);
}

void MainWindow::loadTabFiles() {
  //读取标签页
  QSettings Reg(strIniFile, QSettings::IniFormat);
  int count = Reg.value("count").toInt();
  bool yes = false;

  if (count == 0) {
    newFile("");
    yes = true;
  }

  for (int i = 0; i < count; i++) {
    QString file = Reg.value(QString::number(i) + "/file").toString();
    QFileInfo fi(file);

    if (fi.exists()) {
      int row, col;
      row = Reg.value(QString::number(i) + "/row").toInt();
      col = Reg.value(QString::number(i) + "/col").toInt();

      loadFile(file, row, col);

      int vs, hs;
      vs = Reg.value(QString::number(i) + "/vs").toInt();
      hs = Reg.value(QString::number(i) + "/hs").toInt();

      textEdit->verticalScrollBar()->setSliderPosition(vs);
      textEdit->horizontalScrollBar()->setSliderPosition(hs);
      textEdit->setFocus();

      yes = true;
    } else {
      newFile("");
      yes = true;
    }
  }

  if (!yes) newFile("");

  int tab_total = ui->tabWidget_textEdit->tabBar()->count();  //以实际存在的为准
  int ci = Reg.value("ci").toInt();
  if (ci < tab_total) {
    ui->tabWidget_textEdit->setCurrentIndex(ci);
    on_tabWidget_textEdit_tabBarClicked(ci);
  } else {
    ui->tabWidget_textEdit->setCurrentIndex(tab_total - 1);
    on_tabWidget_textEdit_tabBarClicked(tab_total - 1);
  }
}

void MainWindow::about() {
  QFileInfo appInfo(qApp->applicationFilePath());
  QString str;

  str = tr("Last modified: ");

  QString last = str + appInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
  QString str1 =
      "<a style='color:blue;' href = "
      "https://github.com/ic005k/" +
      strAppName +
      ">Xiasl"
      "</a><br><br>";

  QMessageBox::about(this, "About",
                     str1 + "V" + CurVersion + "<br><br>" + last);
}

QString MainWindow::openFile(QString fileName) {
  removeFilesWatch();

  QFileInfo fInfo(fileName);

  if (fInfo.suffix() == "aml" || fInfo.suffix() == "dat") {
    //如果之前这个文件被打开过，则返回
    QString str = fInfo.path() + "/" + fInfo.baseName() + ".dsl";

    for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
      QWidget* pWidget = ui->tabWidget_textEdit->widget(i);

      lblCurrentFile = (QLabel*)pWidget->children().at(
          lblNumber);  // 2为QLabel,1为textEdit,0为VBoxLayout

      if (str == lblCurrentFile->text()) {
        addFilesWatch();

        return str;
      }
    }

    QFileInfo appInfo(qApp->applicationDirPath());

    Decompile = new QProcess;

    //显示信息窗口并初始化表头
    InfoWinShow = true;
    //标记tab头
    int info_count = 0;

    ui->tabWidget->setTabText(
        1, tr("Errors") + " (" + QString::number(info_count) + ")");
    ui->tabWidget->setTabText(
        2, tr("Warnings") + " (" + QString::number(info_count) + ")");
    ui->tabWidget->setTabText(
        3, tr("Remarks") + " (" + QString::number(info_count) + ")");
    ui->tabWidget->setTabText(4, tr("Scribble"));
    ui->actionInfo_win->setChecked(true);

    ui->listWidget->clear();
    ui->listWidget->addItem(
        new QListWidgetItem(QIcon(":/icon/i10.png"), tr("BasicInfo")));
    ui->listWidget->addItem(new QListWidgetItem(
        QIcon(":/icon/i20.png"), ui->tabWidget->tabBar()->tabText(1)));
    ui->listWidget->addItem(new QListWidgetItem(
        QIcon(":/icon/i30.png"), ui->tabWidget->tabBar()->tabText(2)));
    ui->listWidget->addItem(new QListWidgetItem(
        QIcon(":/icon/i40.png"), ui->tabWidget->tabBar()->tabText(3)));
    ui->listWidget->addItem(new QListWidgetItem(
        QIcon(":/icon/i50.png"), ui->tabWidget->tabBar()->tabText(4)));

    QString name;
    //设置文件过滤器
    QStringList nameFilters;

    if (fInfo.suffix() == "aml") {
      name = "/*.aml";
      //设置文件过滤格式
      nameFilters << "*.aml";
    }
    if (fInfo.suffix() == "dat") {
      name = "/*.dat";
      //设置文件过滤格式
      nameFilters << "*.dat";
    }

    QDir dir(fInfo.path());

    //将过滤后的文件名称存入到files列表中
    QStringList files =
        dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);

    int count = files.count();

    if (!dlgset->ui->chkAll->isChecked()) {
      count = 1;
      files.clear();
      files.append(fInfo.fileName());
    }

    for (int i = 0; i < count; i++) {
      QString dfile = fInfo.path() + "/" + files.at(i);

      try {
#ifdef Q_OS_WIN32

        Decompile->start(appInfo.filePath() + "/iasl.exe",
                         QStringList() << "-d" << dfile);
#endif

#ifdef Q_OS_LINUX

        Decompile->start(appInfo.filePath() + "/iasl", QStringList()
                                                           << "-d" << dfile);

#endif

#ifdef Q_OS_MAC

        Decompile->start(appInfo.filePath() + "/iasl", QStringList()
                                                           << "-d" << dfile);
#endif

        connect(Decompile, SIGNAL(finished(int)), this,
                SLOT(readDecompileResult(int)));

#ifdef Q_OS_WIN32

        Decompile->execute(appInfo.filePath() + "/iasl.exe",
                           QStringList() << "-d" << dfile);
#endif

#ifdef Q_OS_LINUX

        Decompile->execute(appInfo.filePath() + "/iasl", QStringList()
                                                             << "-d" << dfile);

#endif

#ifdef Q_OS_MAC

        Decompile->execute(appInfo.filePath() + "/iasl", QStringList()
                                                             << "-d" << dfile);
#endif
      } catch (...) {
        qDebug() << "error";
        Decompile->terminate();
      }

    }  // for

    Decompile->terminate();

    fileName = fInfo.path() + "/" + fInfo.baseName() + ".dsl";
  }

  QFileInfo fi(fileName);
  if (fi.suffix().toLower() == "dsl") {
    ui->actionAutomatic_Line_Feeds->setChecked(
        false);  //取消自动换行，影响dsl文件开启速度
    textEdit->setWrapMode(QsciScintilla::WrapNone);
  }

  return fileName;
}

void MainWindow::init_listForRecentFile(QString fileName) {
  fileName = QDir::fromNativeSeparators(fileName);
  for (int i = 0; i < recentFileList.count(); i++) {
    if (fileName == recentFileList.at(i)) recentFileList.removeAt(i);
  }
  recentFileList.insert(0, fileName);
  if (recentFileList.count() == 21) recentFileList.removeAt(20);
  init_RecentOpenMenuItem();

  QSettings Reg(strIniFile, QSettings::IniFormat);
  Reg.setValue("RecentOpenFileCount", recentFileList.count());
  for (int i = 0; i < recentFileList.count(); i++) {
    Reg.setValue("RecentOpenFile" + QString::number(i), recentFileList.at(i));
  }
}

void MainWindow::loadFile(const QString& fileName, int row, int col) {
  ui->actionAutomatic_Line_Feeds->setChecked(false);
  init_listForRecentFile(fileName);

  /*如果之前文件已打开，则返回已打开的文件*/
  for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
    QWidget* pWidget = ui->tabWidget_textEdit->widget(i);

    lblCurrentFile = (QLabel*)pWidget->children().at(lblNumber);

    if (fileName == lblCurrentFile->text()) {
      ui->tabWidget_textEdit->setCurrentIndex(i);

      if (!ReLoad) {
        on_tabWidget_textEdit_tabBarClicked(i);
        addFilesWatch();

        return;
      } else {
        textEdit = getCurrentEditor(i);
      }
    }
  }

  if (!ReLoad) newFile(fileName);

  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    QMessageBox::warning(
        this, tr("Application"),
        tr("Cannot read file %1:\n%2.")
            .arg(QDir::toNativeSeparators(fileName), file.errorString()));

    addFilesWatch();

    return;
  }

  QTextStream in(&file);
#ifndef QT_NO_CURSOR
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
#endif

  QString text;
  int ColNum, RowNum;
  if (ReLoad)  //记录重装前的行号
  {
    textEdit->getCursorPosition(&RowNum, &ColNum);
  }

  if (ui->actionUTF_8->isChecked()) in.setCodec("UTF-8");
  if (ui->actionGBK->isChecked()) in.setCodec("GBK");
  text = in.readAll();
  textEdit->setText(text);
  miniEdit->clear();
  miniEdit->setText(text);
  file.close();

  if (row != -1 && col != -1) {
    textEdit->setCursorPosition(row, col);
  }

  if (ReLoad)  //文本重装之后刷新树并回到之前的位置
  {
    on_StartRefreshThread();
    textEdit->setCursorPosition(RowNum, ColNum);
  }

#ifndef QT_NO_CURSOR
  QGuiApplication::restoreOverrideCursor();
#endif

  //给当前tab里面的lbl赋值
  QWidget* pWidget = ui->tabWidget_textEdit->currentWidget();
  lblCurrentFile = (QLabel*)pWidget->children().at(
      lblNumber);  // 2为QLabel,1为textEdit,0为VBoxLayout
  lblCurrentFile->setText(fileName);

  QFileInfo ft(fileName);
  ui->tabWidget_textEdit->tabBar()->setTabToolTip(
      ui->tabWidget_textEdit->currentIndex(), ft.filePath());

  QSettings Reg(strIniFile, QSettings::IniFormat);
  int count = Reg.value("miniMapCount").toInt();
  for (int i = 0; i < count; i++) {
    miniEdit->setHidden(Reg.value("miniMap" + fileName, false).toBool());
    miniEdit->setHidden(false);
    if (miniEdit->isHidden())
      textEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, true);
    else
      textEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, false);
  }

  //为拖拽tab准备拖动后的标题名
  ui->tabWidget_textEdit->currentWidget()->setWindowTitle("        " +
                                                          ft.fileName());

  setCurrentFile(fileName);
  statusBar()->showMessage(
      "                                              " + tr("File loaded"),
      2000);

  ui->editShowMsg->clear();

  ui->treeWidget->clear();
  ui->treeWidget->repaint();
  lblLayer->setText("");
  lblMsg->setText("");

  ReLoad = false;

  QIcon icon(":/icon/md0.svg");
  ui->tabWidget_textEdit->tabBar()->setTabIcon(
      ui->tabWidget_textEdit->currentIndex(), icon);

  updateMd5(fileName);
  addFilesWatch();
  init_TabList();
  getBookmarks();
}

void MainWindow::setRecentFiles(QString fileName) {
  //最近打开的文件
  QSettings settings;
  QFileInfo fInfo(fileName);
  QCoreApplication::setOrganizationName("ic005k");
  QCoreApplication::setOrganizationDomain("github.com/ic005k");
  QCoreApplication::setApplicationName(strAppName);
  settings.setValue("currentDirectory", fInfo.absolutePath());
  // qDebug() << settings.fileName(); //最近打开的文件所保存的位置

  m_recentFiles->setMostRecentFile(fileName);
}

void MainWindow::setCurrentFile(const QString& fileName) {
  curFile = fileName;
  textEdit->setModified(false);
  setWindowModified(false);

  shownName = curFile;
  if (curFile.isEmpty()) shownName = tr("untitled") + ".dsl";

  setWindowFilePath(shownName);

  setWindowTitle(ver + shownName);

  ui->actionGo_to_previous_error->setEnabled(false);
  ui->actionGo_to_the_next_error->setEnabled(false);

  //初始化fsm
  init_fsmSyncOpenedFile(shownName);

  QFileInfo f(shownName);
  if (f.suffix().toLower() == "dsl" || f.suffix().toLower() == "asl" ||
      f.suffix().toLower() == "cpp" || f.suffix().toLower() == "c") {
    ui->actionWrapWord->setChecked(false);  //取消自动换行，影响dsl文件开启速度
    textEdit->setWrapMode(QsciScintilla::WrapNone);

    //设置编译功能使能
    ui->actionCompiling->setEnabled(true);

  } else {
    //设置编译功能屏蔽
    ui->actionCompiling->setEnabled(false);

    ui->frameInfo->setHidden(true);
  }

  ui->tabWidget_textEdit->setTabText(ui->tabWidget_textEdit->currentIndex(),
                                     f.fileName());
}

void MainWindow::set_return_text(QString text) {
  QFontMetrics elideFont(ui->btnReturn->font());
  ui->btnReturn->setText(elideFont.elidedText(
      text, Qt::ElideLeft,
      ui->tabWidget_misc->width() - 100));  //省略号显示在左边
}

void MainWindow::Open() {
  QStringList fileNames = QFileDialog::getOpenFileNames(
      this, "DSDT", "", "DSDT(*.aml *.dsl *.dat *.asl);;All(*.*)");
  for (int i = 0; i < fileNames.count(); i++) {
    if (!fileNames.at(i).isEmpty()) {
      loadFile(openFile(fileNames.at(i)), -1, -1);
      fileName = fileNames.at(i);
    }
  }
}

bool MainWindow::maybeSave(QString info) {
  if (!textEdit->isModified()) return true;

  int ret;
  if (!zh_cn) {
    ret = QMessageBox::warning(
        this, tr("Application"),
        tr("The document has been modified.\n"
           "Do you want to save your changes?\n\n") +
            info,
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

  } else {
    QMessageBox box(QMessageBox::Warning, strAppName,
                    "文件内容已修改，是否保存？\n\n" + info);
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                           QMessageBox::Cancel);
    box.setButtonText(QMessageBox::Save, QString("保 存"));
    box.setButtonText(QMessageBox::Cancel, QString("取 消"));
    box.setButtonText(QMessageBox::Discard, QString("放 弃"));
    ret = box.exec();
  }

  switch (ret) {
    case QMessageBox::Save:
      return Save();
    case QMessageBox::Cancel:
      return false;
    default:
      break;
  }
  return true;
}

bool MainWindow::Save() {
  if (curFile.isEmpty()) {
    return SaveAs();
  } else {
    return saveFile(curFile);
  }
}

bool MainWindow::SaveAs() {
  QFileDialog dialog;
  QString fn = dialog.getSaveFileName(this, "DSDT", "",
                                      "DSDT(*.dsl);;DSDT(*.asl);;All(*.*)");
  if (fn.isEmpty()) return false;

  //去重
  for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
    QWidget* pWidget = ui->tabWidget_textEdit->widget(i);

    lblCurrentFile = (QLabel*)pWidget->children().at(
        lblNumber);  // 2为QLabel,1为textEdit,0为VBoxLayout

    if (fn == lblCurrentFile->text()) {
      ui->tabWidget_textEdit->removeTab(i);
    }
  }

  setRecentFiles(fn);

  return saveFile(fn);
}

bool MainWindow::saveFile(const QString& fileName) {
  removeFilesWatch();

  QString errorMessage;

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  QSaveFile file(fileName);
  if (file.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream out(&file);
    out << textEdit->text();
    if (!file.commit()) {
      errorMessage =
          tr("Cannot write file %1:\n%2.")
              .arg(QDir::toNativeSeparators(fileName), file.errorString());
    }
  } else {
    errorMessage =
        tr("Cannot open file %1 for writing:\n%2.")
            .arg(QDir::toNativeSeparators(fileName), file.errorString());
  }
  QGuiApplication::restoreOverrideCursor();

  if (!errorMessage.isEmpty()) {
    QMessageBox::warning(this, tr("Application"), errorMessage);

    addFilesWatch();

    return false;
  }

  QIcon icon(":/icon/md0.svg");
  ui->tabWidget_textEdit->tabBar()->setTabIcon(
      ui->tabWidget_textEdit->currentIndex(), icon);

  //刷新文件路径
  QWidget* pWidget =
      ui->tabWidget_textEdit->widget(ui->tabWidget_textEdit->currentIndex());
  lblCurrentFile = (QLabel*)pWidget->children().at(lblNumber);
  lblCurrentFile->setText(fileName);
  QFileInfo ft(fileName);
  ui->tabWidget_textEdit->tabBar()->setTabToolTip(
      ui->tabWidget_textEdit->currentIndex(), ft.filePath());

  setCurrentFile(fileName);

  statusBar()->showMessage(tr("File saved"), 2000);

  textEdit->setFocus();

  updateMd5(fileName);
  addFilesWatch();
  init_listForRecentFile(fileName);

  return true;
}

void MainWindow::removeFilesWatch() {
  for (int i = 0; i < openFileList.count(); i++) {
    FileSystemWatcher::removeWatchPath(openFileList.at(i));
  }
}

void MainWindow::addFilesWatch() {
  removeFilesWatch();

  openFileList.clear();

  for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
    FileSystemWatcher::addWatchPath(getCurrentFileName(i));
    openFileList.append(getCurrentFileName(i));
  }
}

void MainWindow::getACPITables(bool ssdt) {
  QFileInfo appInfo(qApp->applicationDirPath());

  QProcess dump;
  QProcess iasl;
  QStringList ssdtFiles;

  QString acpiDir = QDir::homePath() + "/Xiasl/ACPI Tables/";

  QDir dir;
  if (dir.mkpath(acpiDir)) {
  }
  if (dir.mkpath(acpiDir + "temp/")) {
  }

  //设置文件过滤器
  QStringList nameFilters;

#ifdef Q_OS_WIN32
  dir.setCurrent(acpiDir);

  dump.execute(appInfo.filePath() + "/acpidump.exe", QStringList() << "-b");

  dir.setCurrent(acpiDir + "temp/");
  dump.execute(appInfo.filePath() + "/acpidump.exe", QStringList() << "-b");

  //设置文件过滤格式
  nameFilters << "ssdt*.dat";
  //将过滤后的文件名称存入到files列表中
  ssdtFiles =
      dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);

  dir.setCurrent(acpiDir);
  if (!ssdt)
    iasl.execute(appInfo.filePath() + "/iasl.exe",
                 QStringList() << "-e" << ssdtFiles << "-d"
                               << "dsdt.dat");

#endif

#ifdef Q_OS_LINUX

  dump.execute(appInfo.filePath() + "/acpidump", QStringList() << "-b");
  // iasl.execute(appInfo.filePath() + "/iasl", QStringList() << "-d"
  //                                                         << "dsdt.dat");

#endif

#ifdef Q_OS_MAC
  QString strExtBin = appInfo.filePath() + "/patchmatic";

  if (QFile(strExtBin).exists()) {
    dir.setCurrent(acpiDir);
    dump.execute(strExtBin, QStringList() << "-extractall" << acpiDir);

    dir.setCurrent(acpiDir + "temp/");
    dump.execute(strExtBin, QStringList() << "-extract" << acpiDir + "temp/");

    //设置文件过滤格式
    nameFilters << "ssdt*.aml";
    //将过滤后的文件名称存入到files列表中
    ssdtFiles =
        dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);

    dir.setCurrent(acpiDir);
    if (!ssdt)
      iasl.execute(appInfo.filePath() + "/iasl",
                   QStringList() << "-e" << ssdtFiles << "-d"
                                 << "dsdt.aml");
  }

#endif

  deleteDirfile(acpiDir + "temp/");

  //获取当前加载的SSDT列表
  QCoreApplication::setOrganizationName("ic005k");
  QCoreApplication::setOrganizationDomain("github.com/ic005k");
  QCoreApplication::setApplicationName("SSDT");

  m_ssdtFiles->setNumOfRecentFiles(ssdtFiles.count());  //最多显示最近的文件个数

  for (int i = ssdtFiles.count() - 1; i > -1; i--) {
    QFileInfo fInfo(acpiDir + ssdtFiles.at(i));
    QSettings settings;
    settings.setValue("currentDirectory", fInfo.absolutePath());
    // qDebug() << settings.fileName(); //最近打开的文件所保存的位置
    m_ssdtFiles->setMostRecentFile(acpiDir + ssdtFiles.at(i));
  }

  if (!ssdt) {
    loadFile(acpiDir + "dsdt.dsl", -1, -1);

    QString dirAcpi = "file:" + acpiDir;
    QDesktopServices::openUrl(QUrl(dirAcpi, QUrl::TolerantMode));
  }
}

bool MainWindow::DeleteDirectory(const QString& path) {
  if (path.isEmpty()) {
    return false;
  }

  QDir dir(path);
  if (!dir.exists()) {
    return true;
  }

  dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
  QFileInfoList fileList = dir.entryInfoList();
  foreach (QFileInfo fi, fileList) {
    if (fi.isFile()) {
      fi.dir().remove(fi.fileName());
    } else {
      DeleteDirectory(fi.absoluteFilePath());
    }
  }
  return dir.rmpath(dir.absolutePath());
}

int MainWindow::deleteDirfile(QString dirName) {
  QDir directory(dirName);
  if (!directory.exists()) {
    return true;
  }

  QString srcPath = QDir::toNativeSeparators(dirName);
  if (!srcPath.endsWith(QDir::separator())) srcPath += QDir::separator();

  QStringList fileNames = directory.entryList(
      QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
  bool error = false;
  for (QStringList::size_type i = 0; i != fileNames.size(); ++i) {
    QString filePath = srcPath + fileNames.at(i);
    QFileInfo fileInfo(filePath);
    if (fileInfo.isFile() || fileInfo.isSymLink()) {
      QFile::setPermissions(filePath, QFile::WriteOwner);
      if (!QFile::remove(filePath)) {
        error = true;
      }
    } else if (fileInfo.isDir()) {
      if (!deleteDirfile(filePath)) {
        error = true;
      }
    }
  }

  if (!directory.rmdir(QDir::toNativeSeparators(directory.path()))) {
    error = true;
  }
  return !error;
}

void MainWindow::btnGenerate_clicked() { getACPITables(false); }

void MainWindow::btnCompile_clicked() {
  QFileInfo cf_info(curFile);
  if (cf_info.suffix().toLower() != "dsl" &&
      cf_info.suffix().toLower() != "asl" &&
      cf_info.suffix().toLower() != "cpp" &&
      cf_info.suffix().toLower() != "c") {
    return;
  }

  QFileInfo appInfo(qApp->applicationDirPath());
  co = new QProcess;

  // if (!curFile.isEmpty()) Save();
  Save();

  lblMsg->setText(tr("Compiling..."));

  qTime.start();

  if (cf_info.suffix().toLower() == "dsl" ||
      cf_info.suffix().toLower() == "asl") {
    QString op = dlgset->ui->cboxCompilationOptions->currentText().trimmed();

#ifdef Q_OS_WIN32
    co->start(appInfo.filePath() + "/iasl.exe", QStringList() << op << curFile);
#endif

#ifdef Q_OS_LINUX
    co->start(appInfo.filePath() + "/iasl", QStringList() << op << curFile);
#endif

#ifdef Q_OS_MAC
    co->start(appInfo.filePath() + "/iasl", QStringList() << op << curFile);
#endif

    connect(co, SIGNAL(finished(int)), this, SLOT(readResult(int)));
  }

  // cpp
  if (cf_info.suffix().toLower() == "cpp") {
    QDir::setCurrent(QFileInfo(curFile).path());
    QString tName =
        QFileInfo(curFile).path() + "/" + QFileInfo(curFile).baseName();
    co->start("g++", QStringList() << curFile << "-o" << tName);

    connect(co, SIGNAL(finished(int)), this, SLOT(readCppResult(int)));
  }

  // c
  if (cf_info.suffix().toLower() == "c") {
    QDir::setCurrent(QFileInfo(curFile).path());
    QString tName =
        QFileInfo(curFile).path() + "/" + QFileInfo(curFile).baseName();
    co->start("gcc", QStringList() << curFile << "-o" << tName);

    connect(co, SIGNAL(finished(int)), this, SLOT(readCppResult(int)));
  }
}

void MainWindow::setMark() {
  //回到第一行
  QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(0);
  ui->editShowMsg->setTextCursor(QTextCursor(block));

  //将"error"高亮
  QString search_text = "Error";
  if (search_text.trimmed().isEmpty()) {
    QMessageBox::information(this, tr("Empty search field"),
                             tr("The search field is empty."));
  } else {
    QTextDocument* document = ui->editShowMsg->document();
    bool found = false;
    QTextCursor highlight_cursor(document);
    QTextCursor cursor(document);

    cursor.beginEditBlock();
    QTextCharFormat color_format(highlight_cursor.charFormat());
    color_format.setForeground(Qt::red);
    color_format.setBackground(Qt::yellow);

    while (!highlight_cursor.isNull() && !highlight_cursor.atEnd()) {
      //查找指定的文本，匹配整个单词
      highlight_cursor = document->find(search_text, highlight_cursor,
                                        QTextDocument::FindCaseSensitively);
      if (!highlight_cursor.isNull()) {
        if (!found) found = true;

        highlight_cursor.mergeCharFormat(color_format);
      }
    }

    cursor.endEditBlock();
  }
}

void MainWindow::readDecompileResult(int exitCode) {
  loading = true;

  QString result, result1;
  result = QString::fromUtf8(Decompile->readAllStandardOutput());
  result1 = QString::fromUtf8(Decompile->readAllStandardError());

  ui->editShowMsg->clear();
  ui->editShowMsg->append(result);
  ui->editShowMsg->append(result1);

  if (exitCode == 0) {
    //标记tab头
    int info_count = 0;

    ui->tabWidget->setTabText(
        1, tr("Errors") + " (" + QString::number(info_count) + ")");

    ui->tabWidget->setTabText(
        2, tr("Warnings") + " (" + QString::number(info_count) + ")");

    ui->tabWidget->setTabText(
        3, tr("Remarks") + " (" + QString::number(info_count) + ")");

    ui->tabWidget->setTabText(4, tr("Scribble"));

    ui->tabWidget->setCurrentIndex(0);

  } else {
  }

  loading = false;
}

void MainWindow::readCppRunResult(int exitCode) {
  if (exitCode == 0) {
    QString result;

    result = QString::fromUtf8(co->readAll());

    ui->editShowMsg->append(result);
  }
}

void MainWindow::readCppResult(int exitCode) {
  ui->editShowMsg->clear();
  ui->editErrors->clear();
  ui->editRemarks->clear();
  ui->editWarnings->clear();

  //标记tab头
  ui->tabWidget->setTabText(1, tr("Errors"));
  ui->tabWidget->setTabText(2, tr("Warnings"));
  ui->tabWidget->setTabText(3, tr("Remarks"));

  QString result, result2;

  result = QString::fromUtf8(co->readAll());
  result2 = QString::fromUtf8(co->readAllStandardError());

  float a = qTime.elapsed() / 1000.00;
  lblMsg->setText(tr("Compiled") + "(" + QTime::currentTime().toString() +
                  "    " + QString::number(a, 'f', 2) + " s)");

  //清除所有标记
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL);

  if (exitCode == 0) {
    ui->editShowMsg->append(result);
    ui->editShowMsg->append(result2);

    ui->actionGo_to_previous_error->setEnabled(false);
    ui->actionGo_to_the_next_error->setEnabled(false);
    ui->tabWidget->setCurrentIndex(0);
    ui->listWidget->setCurrentRow(0);

    co = new QProcess;
    QString tName =
        QFileInfo(curFile).path() + "/" + QFileInfo(curFile).baseName();
    if (win) tName = tName + ".exe";

    QStringList strList;
    co->start(tName, strList);

    connect(co, SIGNAL(finished(int)), this, SLOT(readCppRunResult(int)));

    if (!zh_cn)
      QMessageBox::information(this, strAppName, "Compilation successful.");
    else {
      QMessageBox message(QMessageBox::Information, strAppName,
                          tr("Compilation successful."));
      message.setStandardButtons(QMessageBox::Ok);
      message.setButtonText(QMessageBox::Ok, QString(tr("Ok")));
      message.exec();
    }
  } else {
    ui->actionGo_to_previous_error->setEnabled(true);
    ui->actionGo_to_the_next_error->setEnabled(true);

    ui->editErrors->append(result);
    ui->editErrors->append(result2);
    ui->tabWidget->setCurrentIndex(1);
    ui->listWidget->setCurrentRow(1);

    //回到第一行
    QTextBlock block = ui->editErrors->document()->findBlockByNumber(0);
    ui->editErrors->setTextCursor(QTextCursor(block));

    goCppNextError();
  }

  ui->frameInfo->setHidden(false);
  InfoWinShow = true;
  ui->actionInfo_win->setChecked(true);

  init_ScrollBox();
}

/*读取编译结果信息dsl*/
void MainWindow::readResult(int exitCode) {
  loading = true;

  textEditTemp->clear();

  QString result, result2;

  result = QString::fromUtf8(co->readAll());
  result2 = QString::fromUtf8(co->readAllStandardError());

  textEditTemp->append(result);

  textEditTemp->append(result2);

  //分离基本信息
  ui->editShowMsg->clear();
  QVector<QString> list;
  for (int i = 0; i < textEditTemp->document()->lineCount(); i++) {
    QString str = textEditTemp->document()->findBlockByNumber(i).text();

    list.push_back(str);
    QString str_sub = str.trimmed();
    if (str_sub.mid(0, 5) == "Error" || str_sub.mid(0, 7) == "Warning" ||
        str_sub.mid(0, 6) == "Remark") {
      for (int j = 0; j < i - 2; j++) ui->editShowMsg->append(list.at(j));

      break;
    }
  }

  //分离信息
  separ_info("Warning", ui->editWarnings);
  separ_info("Remark", ui->editRemarks);
  separ_info("Error", ui->editErrors);
  separ_info("Optimization", ui->editOptimizations);

  //回到第一行
  QTextBlock block = ui->editErrors->document()->findBlockByNumber(0);
  ui->editErrors->setTextCursor(QTextCursor(block));

  //清除所有标记,5号标记为编译错误标记
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL, 5);

  float a = qTime.elapsed() / 1000.00;
  lblMsg->setText(tr("Compiled") + "(" + QTime::currentTime().toString() +
                  "    " + QString::number(a, 'f', 2) + " s)");

  if (exitCode == 0) {
    ui->actionGo_to_previous_error->setEnabled(false);
    ui->actionGo_to_the_next_error->setEnabled(false);
    ui->tabWidget->setCurrentIndex(0);
    ui->listWidget->setCurrentRow(0);
    ui->listWidget->setFocus();

    if (!zh_cn)
      QMessageBox::information(this, "Xiasl", "Compilation successful.");
    else {
      QMessageBox message(QMessageBox::Information, "Xiasl",
                          tr("Compilation successful."));
      message.setStandardButtons(QMessageBox::Ok);
      message.setButtonText(QMessageBox::Ok, QString(tr("Ok")));
      message.exec();
    }

  } else {
    ui->actionGo_to_previous_error->setEnabled(true);
    ui->actionGo_to_the_next_error->setEnabled(true);
    ui->tabWidget->setCurrentIndex(1);
    ui->listWidget->setCurrentRow(1);
    ui->listWidget->setFocus();

    on_btnNextError();
  }

  ui->frameInfo->setHidden(false);
  InfoWinShow = true;
  ui->actionInfo_win->setChecked(true);

  init_ScrollBox();

  loading = false;
}

void MainWindow::textEdit_cursorPositionChanged() {
  int RowNum, ColNum;
  textEdit->getCursorPosition(&RowNum, &ColNum);

  QString msg = tr("Row") + " : " + QString::number(RowNum + 1) + "    " +
                tr("Column") + " : " + QString::number(ColNum);

  locationLabel->setText(msg);

  locationLabel->setAlignment(Qt::AlignCenter);
  locationLabel->setMinimumSize(locationLabel->sizeHint());
  // statusBar()->setStyleSheet(
  //    QString("QStatusBar::item{border: 0px}"));  // 设置不显示label的边框
  statusBar()->setSizeGripEnabled(true);  //设置是否显示右边的大小控制点

  //联动treeWidget
  mem_linkage(ui->treeWidget, RowNum);

  setTextModifyMark();
}

void MainWindow::setTextModifyMark() {
  if (!loading) {
    int i = ui->tabWidget_textEdit->currentIndex();
    textEdit = getCurrentEditor(i);
    if (!textEdit->isModified()) {
      QIcon icon(":/icon/md0.svg");
      ui->tabWidget_textEdit->tabBar()->setTabIcon(i, icon);

    } else {
      QIcon icon(":/icon/md1.svg");
      ui->tabWidget_textEdit->tabBar()->setTabIcon(i, icon);
    }
  }
}

void MainWindow::miniEdit_cursorPositionChanged() {}

/*换行之后，1s后再刷新成员树*/
void MainWindow::timer_linkage() {
  if (!loading) {
    on_RefreshTree();

    timer->stop();
  }
}

/*单击文本任意位置，当前代码块与成员树进行联动*/
void MainWindow::mem_linkage(QTreeWidget* tw, int RowNum) {
  lblLayer->setText("");
  if (QFileInfo(curFile).suffix().toLower() != "dsl" &&
      QFileInfo(curFile).suffix().toLower() != "asl" &&
      tw->topLevelItemCount() == 0) {
    return;
  }

  /*进行联动的条件：装载文件没有进行&成员树不为空&不是始终在同一行里面*/
  if (!loading && tw->topLevelItemCount() > 0 && preRow != RowNum) {
    int treeSn = 0;
    QTreeWidgetItemIterator it(tw);
    textEditBack->setCursorPosition(RowNum, 0);  //后台进行

    preRow = RowNum;

    for (int j = RowNum; j > -1; j--)  //从当前行往上寻找Scope、Device、Method
    {
      QString str = textEditBack->text(j).trimmed();
      if (str.mid(0, 5) == "Scope" || str.mid(0, 5) == "Devic" ||
          str.mid(0, 5) == "Metho") {
        while (*it) {
          treeSn = (*it)->text(1).toInt();

          if (treeSn == j) {
            tw->setCurrentItem((*it));
            //状态栏上显示层次结构
            lblLayer->setText(getLayerName((*it)));

            break;
          }

          ++it;
        }

        break;
      }
    }
  }
}

/*行号区域的宽度：目前在主编辑框内已弃用，为编译输出信息显示预留*/
int CodeEditor::lineNumberAreaWidth() {
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10) {
    max /= 10;
    ++digits;
  }

  return 0;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
  if (dy)
    lineNumberArea->scroll(0, dy);
  else
    lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

  if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent* e) {
  QPlainTextEdit::resizeEvent(e);

  QRect cr = contentsRect();
  lineNumberArea->setGeometry(
      QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
  QPainter painter(lineNumberArea);
  painter.fillRect(event->rect(), Qt::lightGray);

  //![extraAreaPaintEvent_0]

  //![extraAreaPaintEvent_1]
  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top =
      qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());
  //![extraAreaPaintEvent_1]

  //![extraAreaPaintEvent_2]
  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      QString number = QString::number(blockNumber + 1);
      painter.setPen(Qt::black);
      painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                       Qt::AlignRight, number);
    }

    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++blockNumber;
  }
}

void CodeEditor::highlightCurrentLine() {
  QList<QTextEdit::ExtraSelection> extraSelections;

  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor(Qt::yellow).lighter(160);

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
  }

  setExtraSelections(extraSelections);
}

void MainWindow::on_btnReplace() { textEdit->replace(ui->editReplace->text()); }

void MainWindow::ReplaceAll() {
  if (ui->editReplace->text().trimmed() == "") return;

  int row, col;
  textEdit->getCursorPosition(&row, &col);

  QString searchtext = ui->editFind->currentText().trimmed();
  QString replacetext = ui->editReplace->text().trimmed();
  QString document = textEdit->text();
  if (!isCaseSensitive)
    document.replace(searchtext, replacetext, Qt::CaseInsensitive);
  else
    document.replace(searchtext, replacetext);

  textEdit->setText(document);

  textEdit->setCursorPosition(row, col);
}

void MainWindow::forEach(QString str, QString strReplace) {
  if (textEdit->findFirst(str, true, isCaseSensitive, false, true, true)) {
    textEdit->replace(strReplace);
  }
}

void MainWindow::on_StartSearch(QsciScintilla* textEdit, QString file) {
  if (loading) return;

  ui->tabWidget_misc->setCurrentIndex(2);
  ui->editFind->setFocus();

  clearSearchHighlight(textEdit);

  QString str = ui->editFind->currentText().trimmed();
  //正则、大小写、匹配整个词、循环查找、向下或向上：目前已开启向下的循环查找

  highlighsearchtext(str, textEdit, file, true);

  if (textEdit->findFirst(str, true, isCaseSensitive, false, true, true)) {
    if (red < 55) {
      QPalette palette;
      palette.setColor(QPalette::Text, Qt::white);
      ui->editFind->setPalette(palette);

      palette = ui->editFind->palette();
      palette.setColor(QPalette::Base, QColor(50, 50, 50, 255));
      ui->editFind->setPalette(palette);

    } else {
      QPalette palette;
      palette.setColor(QPalette::Text, Qt::black);
      ui->editFind->setPalette(palette);

      palette = ui->editFind->palette();
      palette.setColor(QPalette::Base, Qt::white);
      ui->editFind->setPalette(palette);
    }

  } else {
  }

  find_down = true;
  find_up = false;

  init_ScrollBox();
}

void MainWindow::on_btnFindPrevious() {
  clearSearchHighlight(textEdit);
  ui->treeFind->clear();

  QString name = ui->editFind->currentText().trimmed();
  std::string str = name.toStdString();
  const char* ch = str.c_str();

  int flags;
  if (isCaseSensitive)
    flags = QsciScintilla::SCFIND_MATCHCASE | QsciScintilla::SCFIND_REGEXP;
  else
    flags = QsciScintilla::SCFIND_REGEXP;

  textEdit->SendScintilla(QsciScintilla::SCI_SEARCHANCHOR);
  if (textEdit->SendScintilla(QsciScintilla::SCI_SEARCHPREV, flags, ch) == -1) {
  } else {
    if (red < 55) {
    } else {
    }
  }

  QScrollBar* vscrollbar = new QScrollBar;
  vscrollbar = textEdit->verticalScrollBar();

  QScrollBar* hscrollbar = new QScrollBar;
  hscrollbar = textEdit->horizontalScrollBar();

  int row, col, vs_pos, hs_pos;
  vs_pos = vscrollbar->sliderPosition();
  textEdit->getCursorPosition(&row, &col);
  if (row < vs_pos) vscrollbar->setSliderPosition(row - 5);

  hs_pos = hscrollbar->sliderPosition();
  QPainter p(this);
  QFontMetrics fm = p.fontMetrics();
  QString t = textEdit->text(row).mid(0, col);
  int char_w;

  char_w = fm.averageCharWidth();
  qDebug() << col;
  if (char_w < textEdit->viewport()->width())
    hscrollbar->setSliderPosition(0);
  else
    hscrollbar->setSliderPosition(char_w);

  find_down = false;
  find_up = true;

  int index = ui->tabWidget_textEdit->currentIndex();
  QString file = getCurrentFileName(index);
  highlighsearchtext(name, textEdit, file, true);

  init_ScrollBox();
}

void MainWindow::on_treeWidget_itemClicked(QTreeWidgetItem* item, int column) {
  if (column == 0 && !loading) {
    int lines = item->text(1).toInt();
    textEdit->setCursorPosition(lines, 0);
    textEdit->setFocus();
    init_ScrollBox();
  }
}

void MainWindow::treeWidgetBack_itemClicked(QTreeWidgetItem* item, int column) {
  if (column == 0) {
    int lines = item->text(1).toInt();
    textEdit->setCursorPosition(lines, 0);
    textEdit->setFocus();
  }
}

void MainWindow::on_editShowMsg_cursorPositionChanged() {
  set_cursor_line_color(ui->editShowMsg);
}

void MainWindow::set_cursor_line_color(QTextEdit* edit) {
  QList<QTextEdit::ExtraSelection> extraSelection;
  QTextEdit::ExtraSelection selection;
  QColor lineColor = QColor(255, 255, 0, 50);
  selection.format.setBackground(lineColor);
  selection.format.setProperty(QTextFormat::FullWidthSelection, true);
  selection.cursor = edit->textCursor();
  extraSelection.append(selection);
  edit->setExtraSelections(extraSelection);
}

void MainWindow::goCppPreviousError() {
  miniDlg->close();

  const QTextCursor cursor = ui->editErrors->textCursor();

  int RowNum = cursor.blockNumber() - 2;

  QTextBlock block = ui->editErrors->document()->findBlockByNumber(RowNum);
  ui->editErrors->setTextCursor(QTextCursor(block));

  bool yes = false;

  for (int i = RowNum; i > -1; i--) {
    QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
    ui->editErrors->setTextCursor(QTextCursor(block));

    QString str = ui->editErrors->document()->findBlockByLineNumber(i).text();
    QString sub = str.trimmed();

    if (sub.contains(curFile)) {
      yes = true;

      QTextBlock block = ui->editErrors->document()->findBlockByNumber(i + 1);
      ui->editErrors->setTextCursor(QTextCursor(block));

      QList<QTextEdit::ExtraSelection> extraSelection;
      QTextEdit::ExtraSelection selection;

      QColor lineColor = QColor(Qt::red);
      selection.format.setForeground(Qt::white);
      selection.format.setBackground(lineColor);
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = ui->editErrors->textCursor();
      selection.cursor.clearSelection();
      extraSelection.append(selection);
      ui->editErrors->setExtraSelections(extraSelection);

      //定位到错误行
      getCppErrorLine(i + 1);

      ui->tabWidget->setCurrentIndex(1);
      ui->listWidget->setCurrentRow(1);

      break;
    }
  }

  if (!yes) {
    goCppNextError();
  }
}

void MainWindow::goCppNextError() {
  miniDlg->close();

  const QTextCursor cursor = ui->editErrors->textCursor();
  int RowNum = cursor.blockNumber();

  QTextBlock block = ui->editErrors->document()->findBlockByNumber(RowNum);
  ui->editErrors->setTextCursor(QTextCursor(block));

  bool yes = false;

  for (int i = RowNum; i < ui->editErrors->document()->lineCount(); i++) {
    QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
    ui->editErrors->setTextCursor(QTextCursor(block));

    QString str = ui->editErrors->document()->findBlockByLineNumber(i).text();
    QString sub = str.trimmed();

    if (sub.contains(curFile)) {
      yes = true;

      QTextBlock block = ui->editErrors->document()->findBlockByNumber(i + 1);
      ui->editErrors->setTextCursor(QTextCursor(block));

      QList<QTextEdit::ExtraSelection> extraSelection;
      QTextEdit::ExtraSelection selection;
      QColor lineColor = QColor(Qt::red);
      selection.format.setForeground(Qt::white);
      selection.format.setBackground(lineColor);
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = ui->editErrors->textCursor();
      selection.cursor.clearSelection();
      extraSelection.append(selection);
      ui->editErrors->setExtraSelections(extraSelection);

      //定位到错误行
      getCppErrorLine(i + 1);

      ui->tabWidget->setCurrentIndex(1);
      ui->listWidget->setCurrentRow(1);

      break;
    }
  }

  if (!yes) {
    goCppPreviousError();
  }
}

void MainWindow::on_btnNextError() {
  miniDlg->close();

  const QTextCursor cursor = ui->editErrors->textCursor();
  int RowNum = cursor.blockNumber();

  QTextBlock block = ui->editErrors->document()->findBlockByNumber(RowNum);
  ui->editErrors->setTextCursor(QTextCursor(block));

  for (int i = RowNum + 1; i < ui->editErrors->document()->lineCount(); i++) {
    QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
    ui->editErrors->setTextCursor(QTextCursor(block));

    QString str = ui->editErrors->document()->findBlockByLineNumber(i).text();
    QString sub = str.trimmed();

    if (sub.mid(0, 5) == "Error") {
      QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
      ui->editErrors->setTextCursor(QTextCursor(block));

      QList<QTextEdit::ExtraSelection> extraSelection;
      QTextEdit::ExtraSelection selection;
      QColor lineColor = QColor(Qt::red);
      selection.format.setForeground(Qt::white);
      selection.format.setBackground(lineColor);
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = ui->editErrors->textCursor();
      selection.cursor.clearSelection();
      extraSelection.append(selection);
      ui->editErrors->setExtraSelections(extraSelection);

      //定位到错误行
      getErrorLine(i);

      ui->tabWidget->setCurrentIndex(1);

      break;
    }

    if (i == ui->editShowMsg->document()->lineCount() - 1)
      on_btnPreviousError();
  }
}

void MainWindow::on_btnPreviousError() {
  miniDlg->close();

  const QTextCursor cursor = ui->editErrors->textCursor();
  int RowNum = cursor.blockNumber();

  QTextBlock block = ui->editErrors->document()->findBlockByNumber(RowNum);
  ui->editErrors->setTextCursor(QTextCursor(block));

  for (int i = RowNum - 1; i > -1; i--) {
    QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
    ui->editErrors->setTextCursor(QTextCursor(block));

    QString str = ui->editErrors->document()->findBlockByLineNumber(i).text();
    QString sub = str.trimmed();

    if (sub.mid(0, 5) == "Error") {
      QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
      ui->editErrors->setTextCursor(QTextCursor(block));

      QList<QTextEdit::ExtraSelection> extraSelection;
      QTextEdit::ExtraSelection selection;

      QColor lineColor = QColor(Qt::red);
      selection.format.setForeground(Qt::white);
      selection.format.setBackground(lineColor);
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = ui->editErrors->textCursor();
      selection.cursor.clearSelection();
      extraSelection.append(selection);
      ui->editErrors->setExtraSelections(extraSelection);

      //定位到错误行
      getErrorLine(i);

      ui->tabWidget->setCurrentIndex(1);

      break;
    }

    if (i == 0) on_btnNextError();
  }
}

void MainWindow::gotoLine(QTextEdit* edit) {
  QString text, str2, str3;
  int line = 0;
  bool skip = true;
  const QTextCursor cursor = edit->textCursor();
  int RowNum = cursor.blockNumber();

  text = edit->document()->findBlockByNumber(RowNum).text().trimmed();

  if (text != "") {
    for (int j = 3; j < text.count(); j++) {
      if (text.mid(j, 1) == ":") {
        str2 = text.mid(0, j);
        skip = false;
        break;
      }
    }

    if (skip) {
      //再看看上一行
      text = edit->document()->findBlockByNumber(RowNum - 1).text().trimmed();
      if (text != "") {
        for (int j = 3; j < text.count(); j++) {
          if (text.mid(j, 1) == ":") {
            str2 = text.mid(0, j);

            break;
          }
        }
      }
    }

    for (int k = str2.count(); k > 0; k--) {
      if (str2.mid(k - 1, 1) == " ") {
        str3 = str2.mid(k, str2.count() - k);

        //定位到错误行
        line = str3.toInt();
        textEdit->setCursorPosition(line - 1, 0);

        QString strLine = textEdit->text(line - 1);

        for (int i = 0; i < strLine.count(); i++) {
          QString strSub = strLine.trimmed().mid(0, 1);
          if (strLine.mid(i, 1) == strSub) {
            textEdit->setCursorPosition(line - 1, i);
            break;
          }
        }

        for (int i = 0; i < strLine.count(); i++) {
          if (strLine.mid(i, 1) == "(") {
            textEdit->setCursorPosition(line - 1, i + 1);
            break;
          }
        }

        textEdit->setFocus();

        break;
      }
    }
  }
}

void MainWindow::setErrorMarkers(int linenr) {
  // 目前采用箭头
  // 具体定义在此：https://www.scintilla.org/ScintillaDoc.html#SCI_MARKERDEFINE

  // 下划线
  // textEdit->SendScintilla(QsciScintilla::SCI_STYLESETUNDERLINE, linenr,
  // true);
  // textEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 0,
  // QsciScintilla::SC_MARK_UNDERLINE);

  // 5号标记在前面已定义
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, linenr - 1, 5);
}

void MainWindow::setBookmarks(int linenr) {
  // 注意前后对应，如果不是采用命令发送的消息
  // textEdit->markerAdd(linenr, 4);

  textEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, linenr, 4);
}

void MainWindow::getCppErrorLine(int i) {
  //定位到错误行
  QString str1 =
      ui->editErrors->document()->findBlockByLineNumber(i).text().trimmed();
  QString str2, str3, str4;
  if (str1 != "") {
    for (int j = 0; j < str1.count(); j++) {
      if (str1.mid(j, 1) == ":") {
        str2 = str1.mid(j + 1, str1.count() - j);

        break;
      }
    }

    for (int k = 0; k < str2.count(); k++) {
      if (str2.mid(k, 1) == ":") {
        str3 = str2.mid(0, k);
        str4 = str2.mid(k + 1, str2.count() - k);

        int linenr = str3.toInt();
        int col = 0;

        for (int n = 0; n < str4.count(); n++) {
          if (str4.mid(n, 1) == ":") {
            str4 = str4.mid(0, n);

            col = str4.toInt();
            break;
          }
        }

        //定位到错误行
        textEdit->setCursorPosition(linenr - 1, col - 1);
        setErrorMarkers(linenr);
        textEdit->setFocus();

        break;
      }
    }
  }
}

void MainWindow::getErrorLine(int i) {
  //定位到错误行
  QString str1 =
      ui->editErrors->document()->findBlockByLineNumber(i - 1).text().trimmed();
  QString str2, str3;
  if (str1 != "") {
    for (int j = 3; j < str1.count(); j++) {
      if (str1.mid(j, 1) == ":") {
        str2 = str1.mid(0, j);

        break;
      }
    }

    for (int k = str2.count(); k > 0; k--) {
      if (str2.mid(k - 1, 1) == " ") {
        str3 = str2.mid(k, str2.count() - k);

        int linenr = str3.toInt();

        //定位到错误行
        textEdit->setCursorPosition(linenr - 1, 0);

        QString strLine = textEdit->text(linenr - 1);
        for (int i = 0; i < strLine.count(); i++) {
          QString strSub = strLine.trimmed().mid(0, 1);
          if (strLine.mid(i, 1) == strSub) {
            textEdit->setCursorPosition(linenr - 1, i);
            break;
          }
        }
        for (int i = 0; i < strLine.count(); i++) {
          if (strLine.mid(i, 1) == "(") {
            textEdit->setCursorPosition(linenr - 1, i + 1);
            break;
          }
        }

        setErrorMarkers(linenr);
        textEdit->setFocus();

        break;
      }
    }
  }
}

void MainWindow::on_editShowMsg_selectionChanged() {
  QString row = ui->editShowMsg->textCursor().selectedText();
  int row_num = row.toUInt();
  if (row_num > 0) {
    textEdit->setCursorPosition(row_num - 1, 0);

    textEdit->setFocus();
  }
}

void MainWindow::textEdit_textChanged() {
  if (!loading) {
    if (m_searchTextPosList.count() > 0) {
    }
  }
}

void MainWindow::on_editFind_ReturnPressed() {
  if (ui->treeFind->topLevelItemCount() > 0) {
    on_btnNext_clicked();
    ui->editFind->setFocus();
  } else {
    on_btnSearch_clicked();
  }
  Methods::setSearchHistory();
}

const char* QscilexerCppAttach::keywords(int set) const {
  if (set == 1)
    return "and and_eq asm auto bitand bitor bool break case "
           "catch char class compl const const_cast continue "
           "default delete do double dynamic_cast else enum "
           "explicit export extern false float for friend goto if "
           "inline int long mutable namespace new not not_eq "
           "operator or or_eq private protected public register "
           "reinterpret_cast return short signed sizeof static "
           "static_cast struct switch template this throw true "
           "try typedef typeid typename union unsigned using "
           "virtual void volatile wchar_t while xor xor_eq "

           "External Scope Device Method Name If While Break Return ElseIf "
           "Switch Case Else "
           "Default Field OperationRegion Package DefinitionBlock Offset "
           "CreateDWordField CreateByteField "
           "CreateBitField CreateWordField CreateQWordField Buffer ToInteger "
           "ToString ToUUID ToUuid ToHexString ToDecimalString ToBuffer ToBcd"
           "CondRefOf FindSetLeftBit FindSetRightBit FromBcd Function "
           "CreateField "

           "Acquire Add Alias And "
           "BankField AccessAs CondRefOf ExtendedMemory ExtendedSpace "
           "BreakPoint Concatenate ConcatenateResTemplate Connection Continue "
           "CopyObject DataTableRegion Debug Decrement DerefOf "
           "Divide Dma Arg0 Arg1 Arg2 Arg3 Arg4 Arg5 Arg6 "
           "DWordIo DWordIO EisaId EndDependentFn Event ExtendedIo Fatal "
           "FixedDma FixedIo GpioInt GpioIo "
           "Increment Index IndexField Interrupt Io IO Irq IRQ IrqNoFlags "
           "LAnd LEqual LGreater LGreaterEqual LLess LLessEqual LNot LNotEqual "
           "Load LOr Match Mid Mod Multiply "
           "Mutex NAnd NoOp NOr Not Notify ObjectType Or PowerResource "
           "Revision "
           "Memory32Fixed "
           "DWordMemory Local0 Local1 Local2 Local3 Local4 Local5 Local6 "
           "Local7 "
           "DWordSpace One Ones Processor QWordIo QWordIO Memory24 Memory32 "
           "VendorLong VendorShort Wait WordBusNumber WordIo WordSpace "
           "I2cSerialBusV2 Include LoadTable QWordMemory QWordSpace "
           "RawDataBuffer RefOf Register Release Reset ResourceTemplate "
           "ShiftLeft ShiftRight Signal SizeOf Sleep "
           "SpiSerialBusV2 Stall StartDependentFn StartDependentFnNoPri Store "
           "Subtract ThermalZone Timer ToBcd UartSerialBusV2 Unicode Unload "
           "Xor Zero ";

  if (set == 2)
    return "SubDecode PosDecode AttribBytes SubDecode PosDecode ReadWrite "
           "ReadOnly Width8bit Width16bit Width32bit Width64bit Width128bit "
           "Width256bit "
           "UserDefRegionSpace SystemIO SystemMemory TypeTranslation "
           "TypeStatic AttribRawBytes AttribRawProcessBytes Serialized "
           "NotSerialized "
           "key dict array TypeA TypeB TypeF AnyAcc ByteAcc Cacheable "
           "WriteCombining Prefetchable NonCacheable PullDefault PullUp "
           "PullDown PullNone "
           "MethodObj UnknownObj IntObj DeviceObj MutexObj PkgObj FieldUnitObj "
           "StrObj Edge Level ActiveHigh ActiveLow ActiveBoth "
           "BuffObj EventObj OpRegionObj PowerResObj ProcessorObj "
           "ThermalZoneObj BuffFieldObj DDBHandleObj None ReturnArg "
           "PolarityHigh PolarityLow ThreeWireMode FourWireMode "
           "MinFixed MinNotFixed MaxFixed MaxNotFixed ResourceConsumer "
           "ResourceProducer MinFixed MinNotFixed MaxFixed MaxNotFixed "
           "ClockPolarityLow ClockPolarityHigh "
           "ResourceConsumer ResourceProducer SubDecode PosDecode MaxFixed "
           "MaxNotFixed GeneralPurposeIo GenericSerialBus FFixedHW "
           "ClockPhaseFirst ClockPhaseSecond "
           "MTR MEQ MLE MLT MGE MGT WordAcc DWordAcc QWordAcc BufferAcc Lock "
           "NoLock AddressRangeMemory AddressRangeReserved AddressRangeNVS "
           "AddressRangeACPI FlowControlHardware "
           "AttribQuick AttribSendReceive AttribByte AttribWord AttribBlock "
           "AttribProcessCall AttribBlockProcessCall IoRestrictionNone "
           "IoRestrictionInputOnly IoRestrictionOutputOnly "
           "IoRestrictionNoneAndPreserve "
           "Preserve WriteAsOnes WriteAsZeros Compatibility BusMaster "
           "NotBusMaster Transfer8 Transfer16 Transfer8_16 DataBitsFive "
           "DataBitsSix DataBitsSeven ParityTypeOdd ParityTypeEven "
           "FlowControlNone FlowControlXon "
           "ResourceConsumer ResourceProducer SubDecode PosDecode MinFixed "
           "MinNotFixed PCI_Config EmbeddedControl SMBus SystemCMOS "
           "PciBarTarget IPMI BigEndian LittleEndian ParityTypeNone "
           "ParityTypeSpace ParityTypeMark "
           "ISAOnlyRanges NonISAOnlyRanges EntireRange TypeTranslation "
           "TypeStatic SparseTranslation DenseTranslation DataBitsEight "
           "DataBitsNine StopBitsZero StopBitsOne StopBitsOnePlusHalf "
           "StopBitsTwo "
           "Exclusive SharedAndWake ExclusiveAndWake Shared "
           "ControllerInitiated DeviceInitiated AddressingMode7Bit "
           "AddressingMode10Bit Decode16 Decode10 ";

  if (set == 3)
    return "a addindex addtogroup anchor arg attention author b "
           "brief bug c class code date def defgroup deprecated "
           "dontinclude e em endcode endhtmlonly endif "
           "endlatexonly endlink endverbatim enum example "
           "exception f$ f[ f] file fn hideinitializer "
           "htmlinclude htmlonly if image include ingroup "
           "internal invariant interface latexonly li line link "
           "mainpage name namespace nosubgrouping note overload "
           "p page par param post pre ref relates remarks return "
           "retval sa section see showinitializer since skip "
           "skipline struct subsection test throw todo typedef "
           "union until var verbatim verbinclude version warning "
           "weakgroup $ @ \\ & < > # { }";

  return 0;
}

QString findKey(QString str, QString str_sub, int f_null) {
  int total, tab_count;
  QString strs, space;
  tab_count = 0;
  for (int i = 0; i < str.count(); i++) {
    if (str.mid(i, 1) == str_sub) {
      strs = str.mid(0, i);

      for (int j = 0; j < strs.count(); j++) {
        if (strs.mid(j, 1) == "\t") {
          tab_count = tab_count + 1;
        }
      }

      int str_space = strs.count() - tab_count;
      total = str_space + tab_count * 4 - f_null;

      for (int k = 0; k < total; k++) space = space + " ";

      break;
    }
  }

  return space;
}

void MainWindow::textEdit_linesChanged() {
  if (!loading) {
    timer->start(1000);

    init_Bookmarks();
  }
}

void thread_one::run() {
  if (break_run) {
    return;
  }

  thread_end = false;

  tw_list.clear();

  s_count = 0;
  m_count = 0;
  d_count = 0;
  n_count = 0;

  if (QFileInfo(curFile).suffix().toLower() == "cpp" ||
      QFileInfo(curFile).suffix().toLower() == "c") {
    Methods::getVoidForCpp(textEditBack);
  } else if (QFileInfo(curFile).suffix().toLower() == "dsl" ||
             QFileInfo(curFile).suffix().toLower() == "asl" || curFile == "") {
    getMemberTree(textEditBack);
  }

  QMetaObject::invokeMethod(this, "over");
}

/*线程结束后对成员树进行数据刷新*/
void MainWindow::dealover() {
  update_ui_tree();

  if (!loading) init_ScrollBox();

  thread_end = true;
  break_run = false;
}

void MainWindow::update_member(bool show, QString str_void,
                               QList<QTreeWidgetItem*> tw_list) {
  if (!show) {
    tw_list.clear();
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
      QString str = ui->treeWidget->topLevelItem(i)->text(0).trimmed();
      if (str.mid(0, str_void.count()) == str_void) {
        tw_list.append(ui->treeWidget->takeTopLevelItem(i));
        i = -1;
      }
    }
  } else {
    ui->treeWidget->addTopLevelItems(tw_list);
    ui->treeWidget->sortItems(1, Qt::AscendingOrder);
  }
}

void MainWindow::update_ui_tree() {
  if (break_run) {
    return;
  }

  ui->treeWidget->clear();
  ui->treeWidget->update();
  ui->treeWidget->addTopLevelItems(tw_list);
  ui->treeWidget->expandAll();

  QString lbl = "Scope(" + QString::number(s_count) + ")  " + "Device(" +
                QString::number(d_count) + ")  " + "Method(" +
                QString::number(m_count) +
                ")";  //  + "N(" + QString::number(n_count) + ")"
  ui->treeWidget->setHeaderLabel(lbl);
  ui->lblMembers->setText(lbl);

  // ui->tabWidget_misc->tabBar()->setTabText(0, lbl);

  ui->treeWidget->update();

  float a = qTime.elapsed() / 1000.00;
  lblMsg->setText(tr("Refresh completed") + "(" +
                  QTime::currentTime().toString() + "    " +
                  QString::number(a, 'f', 2) + " s)");

  QFileInfo fi(curFile);
  if (fi.suffix().toLower() == "dsl") {
    ui->treeWidget->setHidden(false);
  }

  int row, col;
  textEdit->getCursorPosition(&row, &col);
  preRow = 0;
  mem_linkage(ui->treeWidget, row);
}

void MainWindow::update_ui_tw() {
  ui->treeWidget->clear();

  ui->treeWidget->update();

  ui->treeWidget->addTopLevelItems(twitems);

  ui->treeWidget->sortItems(1, Qt::AscendingOrder);  //排序

  ui->treeWidget->setIconSize(QSize(12, 12));

  ui->treeWidget->setHeaderLabel("S(" + QString::number(s_count) + ")  " +
                                 "D(" + QString::number(d_count) + ")  " +
                                 "M(" + QString::number(m_count) + ")  " +
                                 "N(" + QString::number(n_count) + ")");
  ui->treeWidget->update();

  float a = qTime.elapsed() / 1000.00;
  lblMsg->setText("Refresh completed(" + QTime::currentTime().toString() +
                  "    " + QString::number(a, 'f', 2) + " s)");

  textEdit_cursorPositionChanged();

  QFileInfo fi(curFile);
  if (fi.suffix().toLower() == "dsl") {
    ui->treeWidget->setHidden(false);
  }
}

void MainWindow::on_StartRefreshThread() {
  if (!thread_end) {
    break_run = true;

    mythread->quit();
    mythread->wait();

    /*等待线程结束,以使最后一次刷新可以完成*/
    while (!thread_end) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
  }

  //将textEdit的内容读到后台
  textEditBack->clear();
  textEditBack->setText(textEdit->text());

  lblMsg->setText(tr("Refreshing..."));

  qTime.start();
  mythread->start();
}

void MainWindow::on_RefreshTree() {
  init_MiniText();
  on_StartRefreshThread();
}

void MainWindow::init_MiniText() {
  miniLexer = init_Lexer(curFile);
  init_MiniEdit();
  int row, col;
  textEdit->getCursorPosition(&row, &col);
  miniEdit->clear();
  miniEdit->setText(textEdit->text());
  miniEdit->setCursorPosition(row, 0);
  miniEdit->p0 = miniEdit->verticalScrollBar()->sliderPosition();

  QString msg = tr("Row") + " : " + QString::number(row + 1) + "    " +
                tr("Column") + " : " + QString::number(col);

  locationLabel->setText(msg);

  init_ScrollBox();
}

QString getMemberName(QString str_member, QsciScintilla* textEdit, int RowNum) {
  QString sub;

  sub = textEdit->text(RowNum).trimmed();

  QString str_end;
  if (sub.mid(0, str_member.count()) == str_member) {
    for (int i = 0; i < sub.count(); i++) {
      if (sub.mid(i, 1) == ")") {
        str_end = sub.mid(0, i + 1);
        break;
      }
    }
  }

  return str_end;
}

void MainWindow::set_mark(int linenr) {
  // SCI_MARKERGET 参数用来设置标记，默认为圆形标记
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERGET, linenr);
  // SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0, QColor(Qt::red));
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0, QColor(Qt::red));
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, linenr);
}

int getBraceScope(int start, int count, QsciScintilla* textEdit) {
  int dkh1 = 0;
  int scope_end = 0;
  bool end = false;
  /*start-1,从当前行就开始解析，囊括Scope(){等这种紧跟{的写法*/
  for (int s = start - 1; s < count; s++) {
    QString str = textEdit->text(s).trimmed();

    for (int t = 0; t < str.count(); t++) {
      if (str.mid(0, 2) != "/*" && str.mid(0, 2) != "//") {
        if (str.mid(t, 1) == "{") {
          dkh1++;
        }
        if (str.mid(t, 1) == "}") {
          dkh1--;

          if (dkh1 == 0) {
            //范围结束
            int row, col;
            textEdit->getCursorPosition(&row, &col);
            scope_end = s + 1;
            end = true;
            // qDebug() << "范围结束" << scope_end;
            break;
          }
        }
      }
    }

    if (end) {
      break;
    }
  }

  /*如果没有找到匹配的}，则返回开始位置的下一行，否则会进行无限循环*/
  if (!end) return start + 1;

  return scope_end;
}

bool chkMemberName(QString str, QString name) {
  if (str.trimmed().mid(0, name.count()) == name) return true;

  return false;
}

void addSubItem(int start, int end, QsciScintilla* textEdit, QString Name,
                QTreeWidgetItem* iTop) {
  textEdit->setCursorPosition(start, 0);

  for (int sdds1 = start; sdds1 < end; sdds1++) {
    if (break_run) break;

    QString str = textEdit->text(sdds1).trimmed();

    if (chkMemberName(str, Name)) {
      QTreeWidgetItem* iSub = new QTreeWidgetItem(
          QStringList() << getMemberName(Name, textEdit, sdds1)
                        << QString("%1").arg(sdds1, 7, 10, QChar('0')));

      if (Name == "Device") {
        iSub->setIcon(0, QIcon(":/icon/d.svg"));
        d_count++;
      }
      if (Name == "Scope") {
        iSub->setIcon(0, QIcon(":/icon/s.svg"));
        s_count++;
      }
      if (Name == "Method") {
        iSub->setIcon(0, QIcon(":/icon/m.svg"));
        m_count++;
      }

      iTop->addChild(iSub);
    }
  }
}

QTreeWidgetItem* addChildItem(int row, QsciScintilla* textEdit, QString Name,
                              QTreeWidgetItem* iTop) {
  QTreeWidgetItem* iSub = new QTreeWidgetItem(
      QStringList() << getMemberName(Name, textEdit, row)
                    << QString("%1").arg(row, 7, 10, QChar('0')));
  if (Name == "Device") {
    iSub->setIcon(0, QIcon(":/icon/d.svg"));
    d_count++;
  }
  if (Name == "Scope") {
    iSub->setIcon(0, QIcon(":/icon/s.svg"));
    s_count++;
  }
  if (Name == "Method") {
    iSub->setIcon(0, QIcon(":/icon/m.svg"));
    m_count++;
  }

  iTop->addChild(iSub);

  return iSub;
}

void getMemberTree(QsciScintilla* textEdit) {
  if (break_run) {
    return;
  }

  tw_list.clear();

  s_count = 0;
  m_count = 0;
  d_count = 0;
  n_count = 0;

  QString str_member;

  int count;  //总行数

  QTreeWidgetItem* twItem0;
  count = textEdit->lines();

  for (int j = 0; j < count; j++) {
    if (break_run) break;

    str_member = textEdit->text(j).trimmed();

    //根"Scope"
    if (chkMemberName(str_member, "Scope")) {
      twItem0 = new QTreeWidgetItem(QStringList()
                                    << getMemberName(str_member, textEdit, j)
                                    << QString("%1").arg(j, 7, 10, QChar('0')));
      twItem0->setIcon(0, QIcon(":/icon/s.svg"));
      // tw->addTopLevelItem(twItem0);
      tw_list.append(twItem0);

      s_count++;

      int c_fw_start = j + 1;
      int c_fw_end = getBraceScope(c_fw_start, count, textEdit);

      //再往下找内部成员

      for (int d = c_fw_start; d < c_fw_end; d++) {
        if (break_run) break;

        QString str = textEdit->text(d).trimmed();

        // Scope-->Device
        if (chkMemberName(str, "Device")) {
          QTreeWidgetItem* twItem1 =
              addChildItem(d, textEdit, "Device", twItem0);

          int d2_start = d + 1;
          int d2_end = getBraceScope(d2_start, count, textEdit);

          for (int m2 = d2_start; m2 < d2_end; m2++) {
            if (break_run) break;

            QString str = textEdit->text(m2).trimmed();
            // Scope-->Device-->Method
            if (chkMemberName(str, "Method")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(m2, textEdit, "Method", twItem1);
              if (twItem2) {
              }
            }

            // Scope-->Device-->Device
            if (chkMemberName(str, "Device")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(m2, textEdit, "Device", twItem1);

              int start = m2 + 1;
              int end = getBraceScope(start, count, textEdit);

              for (int sddm1 = start; sddm1 < end; sddm1++) {
                if (break_run) break;

                QString str = textEdit->text(sddm1).trimmed();

                // Scope-->Device-->Device-->Method
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sddm1, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }

                // Scope-->Device-->Device-->Scope
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sddm1, textEdit, "Scope", twItem2);

                  int start_sdds = sddm1 + 1;
                  int end_sdds = getBraceScope(start_sdds, count, textEdit);

                  for (int sdds = start_sdds; sdds < end_sdds; sdds++) {
                    if (break_run) {
                      break;
                    }

                    QString str = textEdit->text(sdds).trimmed();

                    // S--D--D--S--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sdds, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }

                      int start_sddss = sdds + 1;
                      int end_sddss =
                          getBraceScope(start_sddss, count, textEdit);
                      for (int sddss = start_sddss; sddss < end_sddss;
                           sddss++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sddss);

                        // S--D--D--S--S--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddss, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--S--S--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddss, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--S--S--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddss, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      sdds = end_sddss - 1;
                    }

                    // S--D--D--S--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sdds, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }

                    // Scope-->Device-->Device-->Scope-->Device
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sdds, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }

                      int start_sddsd = sdds + 1;
                      int end_sddsd =
                          getBraceScope(start_sddsd, count, textEdit);
                      for (int sddsd = start_sddsd; sddsd < end_sddsd;
                           sddsd++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sddsd);

                        // S--D--D--S--D--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddsd, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--S--D--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddsd, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--S--D--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddsd, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      sdds = end_sddsd - 1;
                    }
                  }

                  sddm1 = end_sdds - 1;
                }

                // Scope-->Device-->Device-->Device
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sddm1, textEdit, "Device", twItem2);

                  int start3 = sddm1 + 1;
                  int end3 = getBraceScope(start3, count, textEdit);

                  for (int sddd = start3; sddd < end3; sddd++) {
                    QString str = textEdit->text(sddd).trimmed();

                    // Scope-->Device-->Device-->Device-->Method
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sddd, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }

                    // Scope-->Device-->Device-->Device-->Scope
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sddd, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }

                      int start_sddds = sddd + 1;
                      int end_sddds =
                          getBraceScope(start_sddds, count, textEdit);
                      for (int sddds = start_sddds; sddds < end_sddds;
                           sddds++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sddds);

                        // S--D--D--D--S--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddds, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--D--S--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddds, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--D--S--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sddds, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      sddd = end_sddds - 1;
                    }

                    // Scope-->Device-->Device-->Device-->Device
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sddd, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }

                      int start_sdddd = sddd + 1;
                      int end_sdddd =
                          getBraceScope(start_sdddd, count, textEdit);
                      for (int sdddd = start_sdddd; sdddd < end_sdddd;
                           sdddd++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sdddd);

                        // S--D--D--D--D--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdddd, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--D--D--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdddd, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }
                        }

                        // S--D--D--D--D--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdddd, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      sddd = end_sdddd - 1;
                    }
                  }

                  sddm1 = end3 - 1;
                }
              }

              m2 = end - 1;
            }

            // Scope-->Device-->Scope
            if (chkMemberName(str, "Scope")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(m2, textEdit, "Scope", twItem1);
              if (twItem2) {
              }

              int start_sds = m2 + 1;
              int end_sds = getBraceScope(start_sds, count, textEdit);

              for (int sds = start_sds; sds < end_sds; sds++) {
                if (break_run) break;

                QString str = textEdit->text(sds).trimmed();

                // Scope-->Device-->Scope-->Scope
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sds, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }

                  int start_sdss = sds + 1;
                  int end_sdss = getBraceScope(start_sdss, count, textEdit);
                  for (int sdss = start_sdss; sdss < end_sdss; sdss++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(sdss);

                    // S--D--S--S--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sdss, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }

                      int start_sdsss = sdss + 1;
                      int end_sdsss =
                          getBraceScope(start_sdsss, count, textEdit);
                      for (int sdsss = start_sdsss; sdsss < end_sdsss;
                           sdsss++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sdsss);

                        // S--D--S--S--S--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsss, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }

                          int start_sdssss = sdsss + 1;
                          int end_sdssss =
                              getBraceScope(start_sdssss, count, textEdit);
                          for (int sdssss = start_sdssss; sdssss < end_sdssss;
                               sdssss++) {
                            if (break_run) {
                              break;
                            }
                            QString str = textEdit->text(sdssss);

                            // S--D--S--S--S--S--S
                            if (chkMemberName(str, "Scope")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssss, textEdit, "Scope", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--S--S--D
                            if (chkMemberName(str, "Device")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssss, textEdit, "Device", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--S--S--M
                            if (chkMemberName(str, "Method")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssss, textEdit, "Method", twItem5);
                              if (twItem6) {
                              }
                            }
                          }

                          sdsss = end_sdssss - 1;
                        }
                        // S--D--S--S--S--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsss, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }

                          int start_sdsssd = sdsss + 1;
                          int end_sdsssd =
                              getBraceScope(start_sdsssd, count, textEdit);
                          for (int sdsssd = start_sdsssd; sdsssd < end_sdsssd;
                               sdsssd++) {
                            if (break_run) {
                              break;
                            }
                            QString str = textEdit->text(sdsssd);

                            // S--D--S--S--S--D--S
                            if (chkMemberName(str, "Scope")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsssd, textEdit, "Scope", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--S--D--D
                            if (chkMemberName(str, "Device")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsssd, textEdit, "Device", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--S--D--M
                            if (chkMemberName(str, "Method")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsssd, textEdit, "Method", twItem5);
                              if (twItem6) {
                              }
                            }
                          }

                          sdsss = end_sdsssd - 1;
                        }
                        // S--D--S--S--S--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsss, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }
                      sdss = end_sdsss - 1;
                    }

                    // S--D--S--S--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sdss, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }

                      int start_sdssd = sdss + 1;
                      int end_sdssd =
                          getBraceScope(start_sdssd, count, textEdit);
                      for (int sdssd = start_sdssd; sdssd < end_sdssd;
                           sdssd++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sdssd);

                        // S--D--S--S--D--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdssd, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }

                          int start_sdssds = sdssd + 1;
                          int end_sdssds =
                              getBraceScope(start_sdssds, count, textEdit);
                          for (int sdssds = start_sdssds; sdssds < end_sdssds;
                               sdssds++) {
                            if (break_run) {
                              break;
                            }
                            QString str = textEdit->text(sdssds);

                            // S--D--S--S--D--S--S
                            if (chkMemberName(str, "Scope")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssds, textEdit, "Scope", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--D--S--D
                            if (chkMemberName(str, "Device")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssds, textEdit, "Device", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--D--S--M
                            if (chkMemberName(str, "Method")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssds, textEdit, "Method", twItem5);
                              if (twItem6) {
                              }
                            }
                          }

                          sdssd = end_sdssds - 1;
                        }
                        // S--D--S--S--D--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdssd, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }

                          int start_sdssdd = sdssd + 1;
                          int end_sdssdd =
                              getBraceScope(start_sdssdd, count, textEdit);
                          for (int sdssdd = start_sdssdd; sdssdd < end_sdssdd;
                               sdssdd++) {
                            if (break_run) {
                              break;
                            }
                            QString str = textEdit->text(sdssdd);

                            // S--D--S--S--D--D--S
                            if (chkMemberName(str, "Scope")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssdd, textEdit, "Scope", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--D--D--D
                            if (chkMemberName(str, "Device")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssdd, textEdit, "Device", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--S--D--D--M
                            if (chkMemberName(str, "Method")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdssdd, textEdit, "Method", twItem5);
                              if (twItem6) {
                              }
                            }
                          }

                          sdssd = end_sdssdd - 1;
                        }
                        // S--D--S--S--D--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdssd, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      sdss = end_sdssd - 1;
                    }

                    // S--D--S--S--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(sdss, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  sds = end_sdss - 1;
                }

                // Scope-->Device-->Scope-->Device
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sds, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }

                  int start4 = sds + 1;
                  int end4 = getBraceScope(start4, count, textEdit);

                  for (int m4 = start4; m4 < end4; m4++) {
                    if (break_run) break;

                    QString str = textEdit->text(m4).trimmed();

                    // Scope-->Device-->Scope-->Device-->Method
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(m4, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }

                    // Scope-->Device-->Scope-->Device-->Device
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(m4, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }

                      int start_sdsdd = m4 + 1;
                      int end_sdsdd =
                          getBraceScope(start_sdsdd, count, textEdit);
                      for (int sdsdd = start_sdsdd; sdsdd < end_sdsdd;
                           sdsdd++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sdsdd);

                        // S--D--S--D--D--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsdd, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }
                        }
                        // S--D--S--D--D--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsdd, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }
                        }
                        // S--D--S--D--D--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsdd, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      m4 = end_sdsdd - 1;
                    }

                    // Scope-->Device-->Scope-->Device-->Scope
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(m4, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }

                      int start_sdsds = m4 + 1;
                      int end_sdsds =
                          getBraceScope(start_sdsds, count, textEdit);
                      for (int sdsds = start_sdsds; sdsds < end_sdsds;
                           sdsds++) {
                        if (break_run) {
                          break;
                        }
                        QString str = textEdit->text(sdsds);

                        // S--D--S--D--S--S
                        if (chkMemberName(str, "Scope")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsds, textEdit, "Scope", twItem4);
                          if (twItem5) {
                          }

                          int start_sdsdss = sdsds + 1;
                          int end_sdsdss =
                              getBraceScope(start_sdsdss, count, textEdit);
                          for (int sdsdss = start_sdsdss; sdsdss < end_sdsdss;
                               sdsdss++) {
                            if (break_run) {
                              break;
                            }
                            QString str = textEdit->text(sdsdss);

                            // S--D--S--D--S--S--S
                            if (chkMemberName(str, "Scope")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsdss, textEdit, "Scope", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--D--S--S--D
                            if (chkMemberName(str, "Device")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsdss, textEdit, "Device", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--D--S--S--M
                            if (chkMemberName(str, "Method")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsdss, textEdit, "Method", twItem5);
                              if (twItem6) {
                              }
                            }
                          }

                          sdsds = end_sdsdss - 1;
                        }
                        // S--D--S--D--S--D
                        if (chkMemberName(str, "Device")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsds, textEdit, "Device", twItem4);
                          if (twItem5) {
                          }

                          int start_sdsdsd = sdsds + 1;
                          int end_sdsdsd =
                              getBraceScope(start_sdsdsd, count, textEdit);
                          for (int sdsdsd = start_sdsdsd; sdsdsd < end_sdsdsd;
                               sdsdsd++) {
                            if (break_run) {
                              break;
                            }
                            QString str = textEdit->text(sdsdsd);

                            // S--D--S--D--S--D--S
                            if (chkMemberName(str, "Scope")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsdsd, textEdit, "Scope", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--D--S--D--D
                            if (chkMemberName(str, "Device")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsdsd, textEdit, "Device", twItem5);
                              if (twItem6) {
                              }
                            }
                            // S--D--S--D--S--D--M
                            if (chkMemberName(str, "Method")) {
                              QTreeWidgetItem* twItem6 = addChildItem(
                                  sdsdsd, textEdit, "Method", twItem5);
                              if (twItem6) {
                              }
                            }
                          }

                          sdsds = end_sdsdsd - 1;
                        }
                        // S--D--S--D--S--M
                        if (chkMemberName(str, "Method")) {
                          QTreeWidgetItem* twItem5 =
                              addChildItem(sdsds, textEdit, "Method", twItem4);
                          if (twItem5) {
                          }
                        }
                      }

                      m4 = end_sdsds - 1;
                    }
                  }

                  sds = end4 - 1;
                }

                // Scope-->Device-->Scope-->Method
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sds, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              m2 = end_sds - 1;
            }
          }

          d = d2_end - 1;
        }

        // S--S
        if (chkMemberName(str, "Scope")) {
          QTreeWidgetItem* twItem1 =
              addChildItem(d, textEdit, "Scope", twItem0);
          if (twItem1) {
          }

          int start_ss = d + 1;
          int end_ss = getBraceScope(start_ss, count, textEdit);
          for (int ss = start_ss; ss < end_ss; ss++) {
            if (break_run) {
              break;
            }
            QString str = textEdit->text(ss);

            // S--S--S
            if (chkMemberName(str, "Scope")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(ss, textEdit, "Scope", twItem1);
              if (twItem2) {
              }

              int start_sss = ss + 1;
              int end_sss = getBraceScope(start_sss, count, textEdit);
              for (int sss = start_sss; sss < end_sss; sss++) {
                if (break_run) {
                  break;
                }
                QString str = textEdit->text(sss);

                // S--S--S--S
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sss, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }
                }
                // S--S--S--D
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sss, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }
                }
                // S--S--S--M
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(sss, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              ss = end_sss - 1;
            }
            // S--S--D
            if (chkMemberName(str, "Device")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(ss, textEdit, "Device", twItem1);
              if (twItem2) {
              }

              int start_ssd = ss + 1;
              int end_ssd = getBraceScope(start_ssd, count, textEdit);
              for (int ssd = start_ssd; ssd < end_ssd; ssd++) {
                if (break_run) {
                  break;
                }
                QString str = textEdit->text(ssd);

                // S--S--D--S
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(ssd, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }
                }
                // S--S--D--D
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(ssd, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }
                }
                // S--S--D--M
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(ssd, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              ss = end_ssd - 1;
            }
            // S--S--M
            if (chkMemberName(str, "Method")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(ss, textEdit, "Method", twItem1);
              if (twItem2) {
              }
            }
          }

          d = end_ss - 1;
        }

        // S--M
        if (chkMemberName(str, "Method")) {
          QTreeWidgetItem* twItem1 =
              addChildItem(d, textEdit, "Method", twItem0);
          if (twItem1) {
          }
        }
      }

      j = c_fw_end - 1;
    }

    //根下的"Method"
    if (chkMemberName(str_member, "Method")) {
      QTreeWidgetItem* twItem0 = new QTreeWidgetItem(
          QStringList() << getMemberName(str_member, textEdit, j)
                        << QString("%1").arg(j, 7, 10, QChar('0')));
      twItem0->setIcon(0, QIcon(":/icon/m.svg"));
      // tw->addTopLevelItem(twItem0);
      tw_list.append(twItem0);

      m_count++;
    }

    //根下的"Device"
    if (chkMemberName(str_member, "Device")) {
      QTreeWidgetItem* twItem0 = new QTreeWidgetItem(
          QStringList() << getMemberName(str_member, textEdit, j)
                        << QString("%1").arg(j, 7, 10, QChar('0')));
      twItem0->setIcon(0, QIcon(":/icon/d.svg"));
      // tw->addTopLevelItem(twItem0);
      tw_list.append(twItem0);

      d_count++;

      int start_d = j + 1;
      int end_d = getBraceScope(start_d, count, textEdit);

      // qDebug() << start_d << end_d;

      for (int d = start_d; d < end_d; d++) {
        if (break_run) break;

        QString str = textEdit->text(d);

        // D--S
        if (chkMemberName(str, "Scope")) {
          QTreeWidgetItem* twItem1 =
              addChildItem(d, textEdit, "Scope", twItem0);
          if (twItem1) {
          }

          int start_ds = d + 1;
          int end_ds = getBraceScope(start_ds, count, textEdit);
          for (int ds = start_ds; ds < end_ds; ds++) {
            if (break_run) {
              break;
            }
            QString str = textEdit->text(ds);

            // D--S--S
            if (chkMemberName(str, "Scope")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(ds, textEdit, "Scope", twItem1);
              if (twItem2) {
              }

              int start_dss = ds + 1;
              int end_dss = getBraceScope(start_dss, count, textEdit);
              for (int dss = start_dss; dss < end_dss; dss++) {
                if (break_run) {
                  break;
                }
                QString str = textEdit->text(dss);

                // D--S--S--S
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dss, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }

                  int start_dsss = dss + 1;
                  int end_dsss = getBraceScope(start_dsss, count, textEdit);
                  for (int dsss = start_dsss; dsss < end_dsss; dsss++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(dsss);

                    // D--S--S--S--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsss, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--S--S--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsss, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--S--S--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsss, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  dss = end_dsss - 1;
                }
                // D--S--S--D
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dss, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }

                  int start_dssd = dss + 1;
                  int end_dssd = getBraceScope(start_dssd, count, textEdit);
                  for (int dssd = start_dssd; dssd < end_dssd; dssd++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(dssd);

                    // D--S--S--D--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dssd, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--S--D--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dssd, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--S--D--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dssd, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  dss = end_dssd - 1;
                }
                // D--S--S--M
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dss, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              ds = end_dss - 1;
            }
            // D--S--D
            if (chkMemberName(str, "Device")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(ds, textEdit, "Device", twItem1);
              if (twItem2) {
              }

              int start_dsd = ds + 1;
              int end_dsd = getBraceScope(start_dsd, count, textEdit);
              for (int dsd = start_dsd; dsd < end_dsd; dsd++) {
                if (break_run) {
                  break;
                }
                QString str = textEdit->text(dsd);

                // D--S--D--S
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dsd, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }

                  /*下一个子层*/
                  int start_dsds = dsd + 1;
                  int end_dsds = getBraceScope(start_dsds, count, textEdit);
                  for (int dsds = start_dsds; dsds < end_dsds; dsds++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(dsds);

                    // D--S--D--S--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsds, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--D--S--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsds, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--D--S--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsds, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  dsd = end_dsds - 1;
                }
                // D--S--D--D
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dsd, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }

                  /*下一个子层*/
                  int start_dsdd = dsd + 1;
                  int end_dsdd = getBraceScope(start_dsdd, count, textEdit);
                  for (int dsdd = start_dsdd; dsdd < end_dsdd; dsdd++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(dsdd);

                    // D--S--D--D--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsdd, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--D--D--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsdd, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--S--D--D--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dsdd, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  dsd = end_dsdd - 1;
                }
                // D--S--D--M
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dsd, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              ds = end_dsd - 1;
            }
            // D--S--M
            if (chkMemberName(str, "Method")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(ds, textEdit, "Method", twItem1);
              if (twItem2) {
              }
            }
          }

          d = end_ds - 1;
        }

        // D--D
        if (chkMemberName(str, "Device")) {
          QTreeWidgetItem* twItem1 =
              addChildItem(d, textEdit, "Device", twItem0);
          if (twItem1) {
          }

          int start_dd = d + 1;
          int end_dd = getBraceScope(start_dd, count, textEdit);
          for (int dd = start_dd; dd < end_dd; dd++) {
            if (break_run) {
              break;
            }
            QString str = textEdit->text(dd);

            // D--D--S
            if (chkMemberName(str, "Scope")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(dd, textEdit, "Scope", twItem1);
              if (twItem2) {
              }

              /*下一个子层*/
              int start_dds = dd + 1;
              int end_dds = getBraceScope(start_dds, count, textEdit);
              for (int dds = start_dds; dds < end_dds; dds++) {
                if (break_run) {
                  break;
                }
                QString str = textEdit->text(dds);

                // D--D--S--S
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dds, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }

                  /*下一个子层*/
                  int start_ddss = dds + 1;
                  int end_ddss = getBraceScope(start_ddss, count, textEdit);
                  for (int ddss = start_ddss; ddss < end_ddss; ddss++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(ddss);

                    // D--D--S--S--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddss, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--S--S--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddss, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--S--S--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddss, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  dds = end_ddss - 1;
                }
                // D--D--S--D
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dds, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }

                  /*下一个子层*/
                  int start_ddsd = dds + 1;
                  int end_ddsd = getBraceScope(start_ddsd, count, textEdit);
                  for (int ddsd = start_ddsd; ddsd < end_ddsd; ddsd++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(ddsd);

                    // D--D--S--D--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddsd, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--S--D--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddsd, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--S--D--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddsd, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  dds = end_ddsd - 1;
                }
                // D--D--S--M
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(dds, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              dd = end_dds - 1;
            }

            // D--D--D
            if (chkMemberName(str, "Device")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(dd, textEdit, "Device", twItem1);
              if (twItem2) {
              }

              int start_ddd = dd + 1;
              int end_ddd = getBraceScope(start_ddd, count, textEdit);
              for (int ddd = start_ddd; ddd < end_ddd; ddd++) {
                if (break_run) {
                  break;
                }
                QString str = textEdit->text(ddd);

                // D--D--D--S
                if (chkMemberName(str, "Scope")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(ddd, textEdit, "Scope", twItem2);
                  if (twItem3) {
                  }

                  int start_ddds = ddd + 1;
                  int end_ddds = getBraceScope(start_ddds, count, textEdit);
                  for (int ddds = start_ddds; ddds < end_ddds; ddds++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(ddds);

                    // D--D--D--S--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddds, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--D--S--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddds, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--D--S--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(ddds, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  ddd = end_ddds - 1;
                }
                // D--D--D--D
                if (chkMemberName(str, "Device")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(ddd, textEdit, "Device", twItem2);
                  if (twItem3) {
                  }

                  int start_dddd = ddd + 1;
                  int end_dddd = getBraceScope(start_dddd, count, textEdit);
                  for (int dddd = start_dddd; dddd < end_dddd; dddd++) {
                    if (break_run) {
                      break;
                    }
                    QString str = textEdit->text(dddd);

                    // D--D--D--D--S
                    if (chkMemberName(str, "Scope")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dddd, textEdit, "Scope", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--D--D--D
                    if (chkMemberName(str, "Device")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dddd, textEdit, "Device", twItem3);
                      if (twItem4) {
                      }
                    }
                    // D--D--D--D--M
                    if (chkMemberName(str, "Method")) {
                      QTreeWidgetItem* twItem4 =
                          addChildItem(dddd, textEdit, "Method", twItem3);
                      if (twItem4) {
                      }
                    }
                  }

                  ddd = end_dddd - 1;
                }
                // D--D--D--M
                if (chkMemberName(str, "Method")) {
                  QTreeWidgetItem* twItem3 =
                      addChildItem(ddd, textEdit, "Method", twItem2);
                  if (twItem3) {
                  }
                }
              }

              dd = end_ddd - 1;
            }

            // D--D--M
            if (chkMemberName(str, "Method")) {
              QTreeWidgetItem* twItem2 =
                  addChildItem(dd, textEdit, "Method", twItem1);
              if (twItem2) {
              }
            }
          }

          d = end_dd - 1;
        }

        // D--M
        if (chkMemberName(str, "Method")) {
          QTreeWidgetItem* twItem1 =
              addChildItem(d, textEdit, "Method", twItem0);
          if (twItem1) {
          }
        }
      }

      j = end_d - 1;
    }
  }
}

void refreshTree() {
  loading = true;

  twitems.clear();

  s_count = 0;
  m_count = 0;
  d_count = 0;
  n_count = 0;

  //枚举"Scope"
  getMembers("Scope", textEditBack);

  //枚举"Device"
  getMembers("Device", textEditBack);

  //枚举"Method"
  getMembers("Method", textEditBack);

  //枚举"Name"
  getMembers("Name", textEditBack);

  loading = false;
}

void getMembers(QString str_member, QsciScintilla* textEdit) {
  if (break_run) return;

  QString str;
  int RowNum, ColNum;
  int count;  //总行数
  QTreeWidgetItem* twItem0;

  count = textEdit->lines();

  //回到第一行
  textEdit->setCursorPosition(0, 0);

  for (int j = 0; j < count; j++) {
    if (break_run) {
      break;
    }

    //正则、区分大小写、匹配整个单词、循环搜索
    if (textEdit->findFirst(str_member, true, true, true, false)) {
      textEdit->getCursorPosition(&RowNum, &ColNum);

      str = textEdit->text(RowNum);

      QString space = findKey(str, str_member.mid(0, 1), 0);

      QString sub = str.trimmed();

      bool zs = false;  //当前行是否存在注释
      for (int k = ColNum; k > -1; k--) {
        if (str.mid(k - 2, 2) == "//" || str.mid(k - 2, 2) == "/*") {
          zs = true;

          break;
        }
      }

      QString str_end;
      if (sub.mid(0, str_member.count()) == str_member && !zs) {
        for (int i = 0; i < sub.count(); i++) {
          if (sub.mid(i, 1) == ")") {
            str_end = sub.mid(0, i + 1);

            twItem0 = new QTreeWidgetItem(
                QStringList()
                << space + str_end
                << QString("%1").arg(RowNum, 7, 10,
                                     QChar('0')));  // QString::number(RowNum));

            if (str_member == "Scope" && show_s) {
              twItem0->setIcon(0, QIcon(":/icon/s.svg"));
              QFont f;
              f.setBold(true);
              twItem0->setFont(0, f);

              twitems.append(twItem0);

              s_count++;
            }
            if (str_member == "Method" && show_m) {
              twItem0->setIcon(0, QIcon(":/icon/m.svg"));

              twitems.append(twItem0);

              m_count++;
            }
            if (str_member == "Name" && show_n) {
              twItem0->setIcon(0, QIcon(":/icon/n.svg"));

              twitems.append(twItem0);

              n_count++;
            }
            if (str_member == "Device" && show_d) {
              twItem0->setIcon(0, QIcon(":/icon/d.svg"));

              twitems.append(twItem0);

              d_count++;
            }

            break;
          }
        }
      }
    } else
      break;
  }
}

void MainWindow::on_MainWindow_destroyed() {}

void MainWindow::init_info_edit() {
  ui->gridLayout_3->setMargin(0);
  ui->gridLayout_4->setMargin(0);
  ui->gridLayout_5->setMargin(0);
  ui->gridLayout_6->setMargin(0);
  ui->gridLayout_13->setMargin(0);
  ui->frameInfo->layout()->setMargin(0);
  ui->frameInfo->layout()->setSpacing(1);

  ui->listWidget->setFrameShape(QFrame::NoFrame);
  ui->listWidget->setSpacing(0);
  ui->listWidget->setIconSize(QSize(20, 20));
  ui->listWidget->setViewMode(QListView::ListMode);
  ui->listWidget->setFocusPolicy(Qt::NoFocus);

  ui->listWidget->addItem(
      new QListWidgetItem(QIcon(":/icon/1i1.png"), tr("BasicInfo")));
  ui->listWidget->addItem(
      new QListWidgetItem(QIcon(":/icon/1i2.png"), tr("Errors")));
  ui->listWidget->addItem(
      new QListWidgetItem(QIcon(":/icon/1i3.png"), tr("Warnings")));
  ui->listWidget->addItem(
      new QListWidgetItem(QIcon(":/icon/1i4.png"), tr("Remarks")));
  ui->listWidget->addItem(
      new QListWidgetItem(QIcon(":/icon/1i50.png"), tr("Scribble")));

  ui->tabWidget->tabBar()->setHidden(true);
  ui->tabWidget->setCurrentIndex(0);
  ui->listWidget->setCurrentRow(0);

  textEditTemp = new QTextEdit();

  ui->editShowMsg->setLineWrapMode(ui->editShowMsg->NoWrap);
  ui->editShowMsg->setReadOnly(true);

  ui->editErrors->setLineWrapMode(ui->editErrors->NoWrap);
  ui->editErrors->setReadOnly(true);

  ui->editWarnings->setLineWrapMode(ui->editWarnings->NoWrap);
  ui->editWarnings->setReadOnly(true);

  ui->editRemarks->setLineWrapMode(ui->editRemarks->NoWrap);
  ui->editRemarks->setReadOnly(true);

  ui->editOptimizations->setLineWrapMode(ui->editOptimizations->NoWrap);
  ui->editOptimizations->setReadOnly(true);
  ui->tabWidget->removeTab(5);  // No need to "optimize" this for now

  ui->frameInfo->setHidden(true);

  // Loading scribble board files
  ui->editScribble->setPlaceholderText(
      tr("This is a scribble board to temporarily record something, and the "
         "content will be saved and loaded automatically."));
  QString fileScribble =
      QDir::homePath() + "/.config/" + strAppName + "/Scribble.txt";
  QFileInfo fi(fileScribble);
  if (fi.exists()) {
    QFile file(fileScribble);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      QMessageBox::warning(
          this, tr("Application"),
          tr("Cannot read file %1:\n%2.")
              .arg(QDir::toNativeSeparators(fileName), file.errorString()));

    } else {
      QTextStream in(&file);
      in.setCodec("UTF-8");
      QString text = in.readAll();
      ui->editScribble->setPlainText(text);
    }
  }
}

void MainWindow::init_RecentOpenMenuItem() {
  mnuRecentOpenFile->clear();
  for (int i = 0; i < recentFileList.count(); i++) {
    QString file = recentFileList.at(i);
    QAction* act = new QAction(QString::number(i + 1) + " . " + file);
    connect(act, &QAction::triggered,
            [=]() { loadFile(openFile(file), -1, -1); });
    mnuRecentOpenFile->addAction(act);
  }
  mnuRecentOpenFile->addSeparator();
  QAction* actClear = new QAction(tr("Clear List"));
  connect(actClear, &QAction::triggered, [=]() {
    recentFileList.clear();
    mnuRecentOpenFile->clear();

    QSettings Reg(strIniFile, QSettings::IniFormat);
    Reg.setValue("RecentOpenFileCount", recentFileList.count());
  });
  mnuRecentOpenFile->addAction(actClear);
}

void MainWindow::init_recentFiles() {
  //最近打开的文件

  QSettings Reg(strIniFile, QSettings::IniFormat);
  int count = Reg.value("RecentOpenFileCount", 0).toInt();
  for (int i = 0; i < count; i++) {
    QString str = Reg.value("RecentOpenFile" + QString::number(i)).toString();
    recentFileList.append(str);
  }

  mnuRecentOpenFile = new QMenu(tr("Open Recent..."));
  ui->menu_File->insertMenu(ui->actionOpen, mnuRecentOpenFile);
#ifdef Q_OS_MAC
  mnuRecentOpenFile->setAsDockMenu();
#endif
  init_RecentOpenMenuItem();

  m_recentFiles = new RecentFiles(this);

  // SSDT list
  QCoreApplication::setOrganizationName("ic005k");
  QCoreApplication::setOrganizationDomain("github.com/ic005k");
  QCoreApplication::setApplicationName("SSDT");

  m_ssdtFiles = new RecentFiles(this);
  m_ssdtFiles->setTitle(tr("Current SSDT List"));

  if (!linuxOS) {
    m_ssdtFiles->attachToMenuAfterItem(
        ui->menu_Edit, tr("Generate ACPI tables"), SLOT(recentOpen(QString)));

    getACPITables(true);  //获取SSDT列表
  }
}

void MainWindow::init_Tool_UI() {
  if (mac || osx1012) this->setUnifiedTitleAndToolBarOnMac(false);
  ui->toolBar->setHidden(true);
  ui->btnFind->setIcon(QIcon(":/icon/find.png"));
  // ui->tabWidget_misc->setTabIcon(2, QIcon(":/icon/find.png"));
  // ui->tabWidget_misc->setTabText(2, "");
  ui->btnFind->setHidden(true);
  ui->btnCompile->setIcon(QIcon(":/icon/2.png"));
  ui->btnErrorP->setIcon(QIcon(":/icon/1.png"));
  ui->btnErrorN->setIcon(QIcon(":/icon/3.png"));

  // textEdit 标签页
  ui->tabWidget_textEdit->setDocumentMode(false);
  ui->tabWidget_textEdit->tabBar()->installEventFilter(
      this);  //安装事件过滤器以禁用鼠标滚轮切换标签页
  connect(ui->tabWidget_textEdit, SIGNAL(tabCloseRequested(int)), this,
          SLOT(closeTab(int)));
  ui->tabWidget_textEdit->setIconSize(QSize(7, 7));

  // 书签
  ui->btnBookmark->setIcon(QIcon(":/icon/book.png"));
  ui->lblBookmarks->setAlignment(Qt::AlignCenter);
  ui->lblNotes->setAlignment(Qt::AlignCenter);
  ui->frameBook->layout()->setMargin(0);
  ui->frameBook->layout()->setSpacing(1);
  ui->listBook->setStyleSheet(ui->listWidget->styleSheet());
  ui->frameBook->setHidden(true);
  ui->frameBook->setFixedWidth(75);

  QSettings Reg(strIniFile, QSettings::IniFormat);
  int count = Reg.value("bookcount", 0).toInt();
  for (int i = 0; i < count; i++) {
    listBookmarks.append(Reg.value("book" + QString::number(i)).toString());
  }

  ui->listBook->setContextMenuPolicy(Qt::CustomContextMenu);
  QMenu* menu = new QMenu(this);
  QAction* act = new QAction(tr("Delete"), this);
  menu->addAction(act);
  connect(act, &QAction::triggered, [=]() { on_btnDelBook_clicked(); });
  connect(ui->listBook, &QTreeView::customContextMenuRequested,
          [=](const QPoint& pos) {
            Q_UNUSED(pos);
            menu->exec(QCursor::pos());
          });

  // 初始化搜索
  ui->chkSubDir->setHidden(true);
  isIncludeSubDir = true;
  textEditSerach = new QsciScintilla;
  ui->frameInFolder->setHidden(true);
  ui->treeFind->setUniformRowHeights(true);  //加快展开速度
  ui->treeFind->setIconSize(QSize(12, 12));
  ui->treeFind->setHeaderHidden(true);
  ui->treeFind->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeFind->header()->setStretchLastSection(false);

  ui->tabFindReplace->layout()->setMargin(2);
  ui->tabFindReplace->layout()->setSpacing(3);

  ui->btnSave->setIcon(QIcon(":/icon/save.png"));
  ui->btnNew->setIcon(QIcon(":/icon/new.png"));
  ui->btnMiniMap->setIcon(QIcon(":/icon/map.png"));
  ui->btnMiniMap->setHidden(true);

  // TAB List
  ui->btnTabList->setToolTip(tr("TAB List"));
  ui->btnTabList->setIcon(QIcon(":/icon/list.png"));
  ui->btnTabList->setPopupMode(QToolButton::InstantPopup);
  menuTabList = new QMenu(this);
  ui->btnTabList->setMenu(menuTabList);

  // Corner Buttons
  ui->frameFun->layout()->setContentsMargins(5, 5, 1, 5);
  ui->frameMainFun->layout()->setContentsMargins(5, 5, 1, 5);
  ui->frameMainFun->layout()->setSpacing(3);
  ui->tabWidget_textEdit->setCornerWidget(ui->frameMainFun);
  ui->frameFun->layout()->setSpacing(3);
  ui->tabWidget_misc->setCornerWidget(ui->frameFun);

  // Find

  QAction* actClear = new QAction(this);
  actClear->setToolTip(tr("Clear search history"));
  actClear->setIcon(QIcon(":/icon/clear.png"));
  ui->editFind->lineEdit()->addAction(actClear, QLineEdit::LeadingPosition);
  connect(actClear, &QAction::triggered, this, &MainWindow::on_clearFindText);

  ui->editFind->lineEdit()->setPlaceholderText(tr("Find"));
  ui->editFind->lineEdit()->setClearButtonEnabled(true);
  // ui->editFind->setAutoCompletionCaseSensitivity(Qt::CaseSensitive);
  setEditFindCompleter();
  connect(ui->editFind->lineEdit(), &QLineEdit::returnPressed, this,
          &MainWindow::on_editFind_ReturnPressed);
  ui->editReplace->setClearButtonEnabled(true);

  if (red < 55) {
    QPalette palette;
    palette = ui->editFind->palette();
    palette.setColor(QPalette::Base, QColor(50, 50, 50));
    palette.setColor(QPalette::Text, Qt::white);
    ui->editFind->setPalette(palette);

  } else {
    QPalette palette;
    palette = ui->editFind->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    ui->editFind->setPalette(palette);
  }

  // 文件被其它修改后的提示
  ui->frameTip->setAutoFillBackground(true);
  ui->frameTip->setPalette(QPalette(QColor(255, 204, 204)));
  ui->btnYes->setDefault(true);
  ui->frameTip->setHidden(true);
}

void MainWindow::init_menu() {
  // File
  ui->actionNew->setShortcut(tr("ctrl+n"));
  if (mac) ui->actionNew->setIconVisibleInMenu(false);

  ui->actionOpen->setShortcut(tr("ctrl+o"));
  connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::Open);
  if (mac) ui->actionOpen->setIconVisibleInMenu(false);

  ui->actionSave->setShortcut(tr("ctrl+s"));
  connect(ui->actionSave, &QAction::triggered, this, &MainWindow::Save);
  if (mac) ui->actionSave->setIconVisibleInMenu(false);

  ui->actionSaveAs->setShortcut(tr("ctrl+shift+s"));
  connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::SaveAs);
  if (mac) ui->actionSaveAs->setIconVisibleInMenu(false);

  ui->actionOpen_directory->setShortcut(tr("ctrl+0"));
  connect(ui->actionOpen_directory, &QAction::triggered, this,
          &MainWindow::on_actionOpenDir);
  ui->actionPreferences->setMenuRole(QAction::PreferencesRole);
  // Quit
  ui->actionQuit->setMenuRole(QAction::QuitRole);

  // Edit
  ui->actionGenerate->setShortcut(tr("ctrl+g"));
  connect(ui->actionGenerate, &QAction::triggered, this,
          &MainWindow::btnGenerate_clicked);

  // ui->actionDSDecompile->setShortcut(tr("ctrl+l"));
  connect(ui->actionDSDecompile, &QAction::triggered, this,
          &MainWindow::ds_Decompile);
  if (mac) ui->actionDSDecompile->setIconVisibleInMenu(false);

  ui->actionCompiling->setShortcut(tr("ctrl+m"));
  connect(ui->actionCompiling, &QAction::triggered, this,
          &MainWindow::btnCompile_clicked);
  if (mac) ui->actionCompiling->setIconVisibleInMenu(false);

  ui->actionRefreshTree->setShortcut(tr("ctrl+r"));
  connect(ui->actionRefreshTree, &QAction::triggered, this,
          &MainWindow::on_RefreshTree);
  if (mac) ui->actionRefreshTree->setIconVisibleInMenu(false);

  if (mac) ui->actionFindPrevious->setIconVisibleInMenu(false);

  // Find & Replace

  if (mac) ui->actionFindNext->setIconVisibleInMenu(false);
  if (mac) ui->actionFind->setIconVisibleInMenu(false);

  ui->actionReplace->setShortcut(tr("ctrl+k"));
  connect(ui->actionReplace, &QAction::triggered, this,
          &MainWindow::on_btnReplace);
  if (mac) ui->actionReplace->setIconVisibleInMenu(false);

  ui->actionReplace_Find->setShortcut(tr("ctrl+j"));
  connect(ui->actionReplace_Find, &QAction::triggered, this,
          &MainWindow::on_btnReplaceFind);
  if (mac) ui->actionReplace_Find->setIconVisibleInMenu(false);
  connect(ui->actionReplaceAll, &QAction::triggered, this,
          &MainWindow::ReplaceAll);
  if (mac) ui->actionReplaceAll->setIconVisibleInMenu(false);

  ui->actionGo_to_previous_error->setShortcut(tr("ctrl+alt+e"));
  connect(ui->actionGo_to_previous_error, &QAction::triggered, this,
          &MainWindow::on_PreviousError);
  if (mac) ui->actionGo_to_previous_error->setIconVisibleInMenu(false);

  ui->actionGo_to_the_next_error->setShortcut(tr("ctrl+e"));
  connect(ui->actionGo_to_the_next_error, &QAction::triggered, this,
          &MainWindow::on_NextError);
  if (mac) ui->actionGo_to_the_next_error->setIconVisibleInMenu(false);

  connect(ui->actionKextstat, &QAction::triggered, this, &MainWindow::kextstat);

  // View
  connect(ui->actionMembers_win, &QAction::triggered, this,
          &MainWindow::view_mem_list);
  ui->actionMembers_win->setShortcut(tr("ctrl+1"));

  connect(ui->actionInfo_win, &QAction::triggered, this,
          &MainWindow::view_info);
  ui->actionInfo_win->setShortcut(tr("ctrl+2"));

  connect(ui->actionMinimap, &QAction::triggered, this,
          &MainWindow::on_miniMap);
  ui->actionMinimap->setShortcut(tr("ctrl+3"));
  ui->actionMinimap->setVisible(false);

  // Help
  connect(ui->actionCheckUpdate, &QAction::triggered, this,
          &MainWindow::CheckUpdate);
  connect(ui->actioniasl_usage, &QAction::triggered, this,
          &MainWindow::iaslUsage);
  connect(ui->actionAbout_1, &QAction::triggered, this, &MainWindow::about);
  ui->actionAbout_1->setMenuRole(QAction::AboutRole);

  QIcon icon;

  icon.addFile(":/icon/return.png");
  ui->btnReturn->setIcon(icon);

  dlgset->ui->cboxCompilationOptions->addItem("-f");
  dlgset->ui->cboxCompilationOptions->addItem("-tp");
  dlgset->ui->cboxCompilationOptions->setEditable(true);

  //读取编译参数

  QFileInfo fi(strIniFile);

  if (fi.exists()) {
    // QSettings Reg(qfile, QSettings::NativeFormat);
    QSettings Reg(strIniFile, QSettings::IniFormat);
    QString op = Reg.value("options").toString().trimmed();
    if (op.count() > 0) dlgset->ui->cboxCompilationOptions->setCurrentText(op);

    //编码
    if (dlgset->ui->rbtnUTF8->isChecked()) lblEncoding->setText("UTF-8");

    if (dlgset->ui->rbtnGBK->isChecked()) lblEncoding->setText("GBK");
  }

  //设置编译功能屏蔽
  ui->actionCompiling->setEnabled(false);
}

int MainWindow::get_Red() {
  //获取背景色
  QPalette pal = ui->treeWidget->palette();
  QBrush brush = pal.window();
  int red = brush.color().red();
  return red;
}

void MainWindow::set_MyStyle(QsciLexer* textLexer, QsciScintilla* textEdit) {
  //获取背景色
  QPalette pal = ui->treeWidget->palette();
  QBrush brush = pal.window();
  red = brush.color().red();

  //设置行号栏宽度、颜色、字体
  QFont m_font = get_Font();

#ifdef Q_OS_WIN32
  textEdit->setMarginWidth(0, 50);
  m_font.setPointSize(9);
#endif

#ifdef Q_OS_LINUX
  textEdit->setMarginWidth(0, 60);
  m_font.setPointSize(10);
#endif

#ifdef Q_OS_MAC
  textEdit->setMarginWidth(0, 55);
  m_font.setPointSize(12);
#endif

  textEdit->setMarginsFont(m_font);

  Methods::init_Margin(textEdit);

  Methods::setColorMatch(red, textLexer);

  //匹配大小括弧
  textEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
  // textEdit->setBraceMatching(QsciScintilla::StrictBraceMatch));//不推荐
  if (red > 55)  //亮模式，mac下阈值为50
  {
    textEdit->setMatchedBraceBackgroundColor(QColor(Qt::green));
    textEdit->setMatchedBraceForegroundColor(QColor(Qt::red));
  }

  //设置括号等自动补全
  textEdit->setAutoIndent(true);
  textEdit->setTabIndents(
      true);  // true如果行前空格数少于tabWidth，补齐空格数,false如果在文字前tab同true，如果在行首tab，则直接增加tabwidth个空格

  //代码提示
  QsciAPIs* apis = new QsciAPIs(textLexer);
  if (apis->load(":/data/apis.txt")) {
  } else
    apis->add(QString("Device"));

  apis->prepare();

  //设置自动补全
  textEdit->setCaretLineVisible(true);
  // Ascii、None、All、Document|APIs
  //禁用自动补全提示功能、所有可用的资源、当前文档中出现的名称都自动补全提示、使用QsciAPIs类加入的名称都自动补全提示
  textEdit->setAutoCompletionSource(
      QsciScintilla::AcsAll);  //自动补全,对于所有Ascii字符
  textEdit->setAutoCompletionCaseSensitivity(false);  //大小写敏感度
  textEdit->setAutoCompletionThreshold(2);  //从第几个字符开始出现自动补全的提示
  // textEdit->setAutoCompletionReplaceWord(false);//是否用补全的字符串替代光标右边的字符串

  //设置缩进参考线
  textEdit->setIndentationGuides(true);
  // textEdit->setIndentationGuidesBackgroundColor(QColor(Qt::white));
  // textEdit->setIndentationGuidesForegroundColor(QColor(Qt::red));

  //设置光标颜色
  if (red < 55)  //暗模式，mac下为50
    textEdit->setCaretForegroundColor(QColor(Qt::white));
  else
    textEdit->setCaretForegroundColor(QColor(Qt::black));
  textEdit->setCaretWidth(2);

  //颜色模式匹配，暗模式，mac下为50
  if (red < 55) {
    //设置光标所在行背景色
    textEdit->setCaretLineBackgroundColor(QColor(180, 180, 0));
    textEdit->setCaretLineFrameWidth(1);
    textEdit->setCaretLineVisible(true);

    textEdit->setMarginsBackgroundColor(QColor(50, 50, 50));
    textEdit->setMarginsForegroundColor(Qt::white);

    textEdit->setFoldMarginColors(Qt::gray, Qt::black);
    // textEdit->setMarginsForegroundColor(Qt::red);  //行号颜色
    textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDFLAGS,
                            16);  //设置折叠标志
    // textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDMARGINCOLOUR,Qt::red);

  } else {
    textEdit->setCaretLineBackgroundColor(QColor(255, 255, 0, 50));
    textEdit->setCaretLineFrameWidth(0);
    textEdit->setCaretLineVisible(true);

    textEdit->setMarginsBackgroundColor(brush.color());
    textEdit->setMarginsForegroundColor(Qt::black);

    textEdit->setFoldMarginColors(Qt::gray, Qt::white);  //折叠栏颜色
    // textEdit->setMarginsForegroundColor(Qt::blue); //行号颜色
    textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDFLAGS,
                            16);  //设置折叠标志
  }

  // 断点设置区域,为后面可能会用到的功能预留
  /*textEdit->setMarginType(1, QsciScintilla::SymbolMargin);
  textEdit->setMarginLineNumbers(1, false);
  textEdit->setMarginWidth(1, 20);
  textEdit->setMarginSensitivity(1, true);  //设置是否可以显示断点
  textEdit->setMarginsBackgroundColor(QColor("#bbfaae"));
  textEdit->setMarginMarkerMask(1, 0x02);
  connect(textEdit, SIGNAL(marginClicked(int, int, Qt::KeyboardModifiers)),
          this, SLOT(on_margin_clicked(int, int, Qt::KeyboardModifiers)));
  textEdit->markerDefine(QsciScintilla::Circle, 1);
  textEdit->setMarkerBackgroundColor(QColor("#ee1111"), 1);*/

  //单步执行显示区域
  /*textEdit->setMarginType(2, QsciScintilla::SymbolMargin);
  textEdit->setMarginLineNumbers(2, false);
  textEdit->setMarginWidth(2, 20);
  textEdit->setMarginSensitivity(2, false);
  textEdit->setMarginMarkerMask(2, 0x04);
  textEdit->markerDefine(QsciScintilla::RightArrow, 2);
  textEdit->setMarkerBackgroundColor(QColor("#eaf593"), 2);*/
}

void MainWindow::init_MiniEdit() {
  // miniEdit = new MiniEditor(this);
  miniEdit->setFrameShape(QFrame::NoFrame);
  miniEdit->verticalScrollBar()->installEventFilter(this);

#ifdef Q_OS_WIN32
  miniEdit->setFixedWidth(60);
  miniEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, false);
#endif

#ifdef Q_OS_LINUX
  miniEdit->setFixedWidth(60);
  miniEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, false);
#endif

#ifdef Q_OS_MAC
  miniEdit->setFixedWidth(60);
  miniEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, false);
#endif

  // miniEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, true);

  miniEdit->setMarginWidth(0, 0);
  miniEdit->setMargins(0);
  miniEdit->setReadOnly(true);
  miniEdit->SendScintilla(QsciScintillaBase::SCI_SETCURSOR, 0, 7);
  miniEdit->setWrapMode(QsciScintilla::WrapNone);
  miniEdit->setCaretWidth(0);
  miniEdit->setCaretLineVisible(false);

  Methods::setColorMatch(red, miniLexer);

  QFont minifont;
  if (win) minifont.setFamily("Agency FB");
  minifont.setPointSizeF(1);
  minifont.setWordSpacing(-6);
  miniLexer->setFont(minifont);
  miniEdit->setFont(minifont);
  miniEdit->setLexer(miniLexer);

  //水平滚动棒
  miniEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTH, -1);
  miniEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTHTRACKING, false);
  miniEdit->horizontalScrollBar()->setHidden(true);

  // miniEdit->SendScintilla(QsciScintilla::SCI_SETMOUSEDOWNCAPTURES, 0, false);

  connect(miniEdit, &QsciScintilla::cursorPositionChanged, this,
          &MainWindow::miniEdit_cursorPositionChanged);
  // connect(miniEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this,
  // SLOT(setValue()));
  connect(miniEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), miniEdit,
          SLOT(miniEdit_verticalScrollBarChanged()));

  //接受文件拖放打开
  miniEdit->setAcceptDrops(false);
  this->setAcceptDrops(true);
}

void MainWindow::init_Edit() {
  textEdit = new QsciScintilla(this);
  textEdit->setFrameShape(QFrame::NoFrame);
  textEdit->installEventFilter(this);
  textEditBack = new QsciScintilla();

  textEdit->setWrapMode(QsciScintilla::WrapNone);

  //设置编码为UTF-8
  textEdit->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,
                          QsciScintilla::SC_CP_UTF8);

  textEdit->setTabWidth(4);

  // 水平滚动条:暂时关闭下面两行代码，否则没法设置水平滚动条的数据
  // textEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTH,
  // textEdit->viewport()->width());
  // textEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTHTRACKING, true);

  // 垂直滚动棒是否看见
  textEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, false);

  connect(textEdit, &QsciScintilla::cursorPositionChanged, this,
          &MainWindow::textEdit_cursorPositionChanged);
  connect(textEdit, &QsciScintilla::textChanged, this,
          &MainWindow::textEdit_textChanged);
  connect(textEdit, &QsciScintilla::linesChanged, this,
          &MainWindow::textEdit_linesChanged);

  connect(textEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(setValue2()));

  QFont font = get_Font();

  textEdit->setFont(font);
  textEdit->setMarginsFont(font);
  myTextLexer->setFont(font);
  textEdit->setLexer(myTextLexer);

  //接受文件拖放打开
  textEdit->setAcceptDrops(false);
  this->setAcceptDrops(true);

  set_MyStyle(myTextLexer, textEdit);
}

QFont MainWindow::get_Font() {
  //读取字体
  QFont font;

  QSettings Reg(strIniFile, QSettings::IniFormat);
  if (mac || osx1012) font.setFamily(Reg.value("FontName", "Menlo").toString());
  if (win) font.setFamily(Reg.value("FontName", "consolas").toString());
  if (linuxOS) font.setFamily(Reg.value("FontName").toString());

  font.setPointSize(Reg.value("FontSize", 12).toInt());
  font.setBold(Reg.value("FontBold").toBool());
  font.setItalic(Reg.value("FontItalic").toBool());
  font.setUnderline(Reg.value("FontUnderline").toBool());

  return font;
}

void MainWindow::init_treeWidget() {
  int w;
  QScreen* screen = QGuiApplication::primaryScreen();
  w = screen->size().width();
  Q_UNUSED(w);

  QFont font = get_Font();
  ui->treeView->setFont(font);
  ui->treeWidget->setFont(font);
  if (!linuxOS) {
    ui->treeWidget->setFont(font.family());
  }

  treeWidgetBak = new QTreeWidget;

  //设置水平滚动条
  ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeWidget->header()->setStretchLastSection(true);

  ui->treeWidget->setFont(font);
  QFont hFont;
  hFont.setPointSize(font.pointSize() - 1);
  ui->treeWidget->header()->setFont(hFont);
  ui->treeWidget->setIconSize(QSize(12, 12));

  ui->treeWidget->setHeaderHidden(true);

  ui->treeWidget->setColumnCount(2);
  ui->treeWidget->setColumnHidden(1, true);

  ui->treeWidget->setColumnWidth(1, 100);
  ui->treeWidget->setHeaderItem(
      new QTreeWidgetItem(QStringList() << tr("Members") << "Lines"));

  ui->treeWidget->setFocusPolicy(
      Qt::NoFocus);  // 去掉选中时的虚线,主要针对windows

  ui->treeWidget->installEventFilter(this);
  if (!win) ui->treeWidget->setAlternatingRowColors(true);  //底色交替显示

  //右键菜单
  ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  QMenu* menu = new QMenu(this);

  QAction* actionExpandAll = new QAction(tr("Expand all"), this);
  menu->addAction(actionExpandAll);
  connect(actionExpandAll, &QAction::triggered, this,
          &MainWindow::on_actionExpandAll);

  QAction* actionCollapseAll = new QAction(tr("Collapse all"), this);
  menu->addAction(actionCollapseAll);
  connect(actionCollapseAll, &QAction::triggered, this,
          &MainWindow::on_actionCollapseAll);

  connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
          [=](const QPoint& pos) {
            Q_UNUSED(pos);

            menu->exec(QCursor::pos());
          });

  ui->lblMembers->setHidden(false);
  ui->lblMembers->setAlignment(Qt::AlignCenter);
}

void MainWindow::init_filesystem() {
  ui->treeView->installEventFilter(this);       //安装事件过滤器
  ui->treeView->setAlternatingRowColors(true);  //不同的底色交替显示
  // ui->treeView->setIconSize(QSize(15, 15));     // 文件浏览器行高

  ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
  QMenu* menu = new QMenu(this);
  QAction* actionOpenDir = new QAction(tr("Open directory"), this);
  menu->addAction(actionOpenDir);
  connect(actionOpenDir, &QAction::triggered, this,
          &MainWindow::on_actionOpenDir);
  connect(ui->treeView, &QTreeView::customContextMenuRequested,
          [=](const QPoint& pos) {
            Q_UNUSED(pos);
            menu->exec(QCursor::pos());
          });

  model = new QFileSystemModel;

#ifdef Q_OS_WIN32
  model->setRootPath("");
#endif

#ifdef Q_OS_LINUX
  model->setRootPath("");
#endif

#ifdef Q_OS_MAC
  model->setRootPath("/Volumes");
#endif

  ui->treeView->setModel(model);
  ui->treeView->setColumnWidth(3, 200);  //注意顺序
  ui->treeView->header()->setSectionResizeMode(
      QHeaderView::ResizeToContents);  //表头列宽自适应

  ui->treeView->setAnimated(false);
  ui->treeView->setIndentation(20);
  ui->treeView->setSortingEnabled(true);
  // const QSize availableSize =
  // ui->treeView->screen()->availableGeometry().size();
  const QSize availableSize = ui->treeView->geometry().size();
  ui->treeView->resize(availableSize / 2);
  ui->treeView->setColumnWidth(0, ui->treeView->width() / 3);

  ui->tabWidget_misc->setCurrentIndex(0);

  //读取之前的目录

  QFileInfo fi(strIniFile);

  if (fi.exists()) {
    QSettings Reg(strIniFile, QSettings::IniFormat);

    // 主窗口位置和大小
    int x, y, width, height;
    x = Reg.value("x", 0).toInt();
    y = Reg.value("y", 0).toInt();
    width = Reg.value("width", 1200).toInt();
    height = Reg.value("height", 600).toInt();
    if (x < 0) {
      width = width + x;
      x = 0;
    }
    if (y < 0) {
      height = height + y;
      y = 0;
    }
    QRect rect(x, y, width, height);
    move(rect.topLeft());
    resize(rect.size());
  }
}

void MainWindow::separ_info(QString str_key, QTextEdit* editInfo) {
  editInfo->clear();

  QTextBlock block = textEditTemp->document()->findBlockByNumber(0);
  textEditTemp->setTextCursor(QTextCursor(block));

  int info_count = 0;
  for (int i = 0; i < textEditTemp->document()->lineCount(); i++) {
    QTextBlock block = textEditTemp->document()->findBlockByNumber(i);
    textEditTemp->setTextCursor(QTextCursor(block));

    QString str = textEditTemp->document()->findBlockByLineNumber(i).text();
    QString sub = str.trimmed();

    if (sub.mid(0, str_key.count()) == str_key) {
      QString str0 = textEditTemp->document()->findBlockByNumber(i - 1).text();
      editInfo->append(str0);

      editInfo->append(str);
      editInfo->append("");

      info_count++;
    }
  }

  //标记tab头

  if (str_key == "Error") {
    ui->tabWidget->setTabText(
        1, tr("Errors") + " (" + QString::number(info_count) + ")");
  }

  if (str_key == "Warning") {
    ui->tabWidget->setTabText(
        2, tr("Warnings") + " (" + QString::number(info_count) + ")");
  }

  if (str_key == "Remark") {
    ui->tabWidget->setTabText(
        3, tr("Remarks") + " (" + QString::number(info_count) + ")");
  }

  if (str_key == "Optimization") ui->tabWidget->setTabText(4, tr("Scribble"));

  ui->listWidget->clear();
  ui->listWidget->addItem(
      new QListWidgetItem(QIcon(":/icon/i10.png"), tr("BasicInfo")));
  ui->listWidget->addItem(new QListWidgetItem(
      QIcon(":/icon/i20.png"), ui->tabWidget->tabBar()->tabText(1)));
  ui->listWidget->addItem(new QListWidgetItem(
      QIcon(":/icon/i30.png"), ui->tabWidget->tabBar()->tabText(2)));
  ui->listWidget->addItem(new QListWidgetItem(
      QIcon(":/icon/i40.png"), ui->tabWidget->tabBar()->tabText(3)));
  ui->listWidget->addItem(new QListWidgetItem(
      QIcon(":/icon/i50.png"), ui->tabWidget->tabBar()->tabText(4)));
}

void MainWindow::on_editErrors_cursorPositionChanged() {
  if (!loading) {
    set_cursor_line_color(ui->editErrors);
    gotoLine(ui->editErrors);
  }
}

void MainWindow::on_editWarnings_cursorPositionChanged() {
  if (!loading) {
    set_cursor_line_color(ui->editWarnings);
    gotoLine(ui->editWarnings);
  }
}

void MainWindow::on_editRemarks_cursorPositionChanged() {
  if (!loading) {
    set_cursor_line_color(ui->editRemarks);
    gotoLine(ui->editRemarks);
  }
}

void MainWindow::on_editOptimizations_cursorPositionChanged() {
  set_cursor_line_color(ui->editOptimizations);
  gotoLine(ui->editOptimizations);
}

void MainWindow::regACPI_win() {
  QString appPath = qApp->applicationFilePath();

  QString dir = qApp->applicationDirPath();
  //注意路径的替换
  appPath.replace("/", "\\");
  QString type = strAppName;
  QSettings* regType =
      new QSettings("HKEY_CLASSES_ROOT\\.dsl", QSettings::NativeFormat);
  QSettings* regIcon = new QSettings("HKEY_CLASSES_ROOT\\.dsl\\DefaultIcon",
                                     QSettings::NativeFormat);
  QSettings* regShell = new QSettings(
      "HKEY_CLASSES_ROOT\\" + strAppName + "\\shell\\open\\command",
      QSettings::NativeFormat);

  QSettings* regType1 =
      new QSettings("HKEY_CLASSES_ROOT\\.aml", QSettings::NativeFormat);
  QSettings* regIcon1 = new QSettings("HKEY_CLASSES_ROOT\\.aml\\DefaultIcon",
                                      QSettings::NativeFormat);
  QSettings* regShell1 = new QSettings(
      "HKEY_CLASSES_ROOT\\" + strAppName + "\\shell\\open\\command",
      QSettings::NativeFormat);

  regType->remove("Default");
  regType->setValue("Default", type);

  regType1->remove("Default");
  regType1->setValue("Default", type);

  regIcon->remove("Default");
  // 0 使用当前程序内置图标
  regIcon->setValue("Default", appPath + ",1");

  regIcon1->remove("Default");
  // 0 使用当前程序内置图标
  regIcon1->setValue("Default", appPath + ",0");

  // 百分号问题
  QString shell = "\"" + appPath + "\" ";
  shell = shell + "\"%1\"";

  regShell->remove("Default");
  regShell->setValue("Default", shell);

  regShell1->remove("Default");
  regShell1->setValue("Default", shell);

  delete regIcon;
  delete regShell;
  delete regType;

  delete regIcon1;
  delete regShell1;
  delete regType1;
  // 通知系统刷新
#ifdef Q_OS_WIN32
  //::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST|SHCNF_FLUSH, 0, 0);
#endif
}

void MainWindow::closeEvent(QCloseEvent* event) {
  //存储编译选项

  QSettings Reg(strIniFile, QSettings::IniFormat);

  Reg.setValue("options",
               dlgset->ui->cboxCompilationOptions->currentText().trimmed());

  //存储搜索历史文本
  int count = ui->editFind->count();

  if (count > 200) count = 200;
  for (int i = 0; i < count; i++) {
    Reg.setValue("FindText" + QString::number(i + 1),
                 ui->editFind->itemText(i));
  }
  Reg.setValue("countFindText", count);

  // 存储窗口大小和位置
  Reg.setValue("x", this->x());
  Reg.setValue("y", this->y());
  Reg.setValue("width", this->width());
  Reg.setValue("height", this->height());

  //存储minimap
  // Reg.setValue("miniMapCount", ui->tabWidget_textEdit->count());
  // for (int i = 0; i < ui->tabWidget_textEdit->count(); i++) {
  //  Reg.setValue("miniMap" + getCurrentFileName(i),
  //               getCurrentMiniEditor(i)->isHidden());
  // }

  //存储编码选项
  Reg.setValue("utf-8", ui->actionUTF_8->isChecked());
  Reg.setValue("gbk", ui->actionGBK->isChecked());

  //存储分割窗口的宽度和高度
  Reg.setValue("w0", ui->tabWidget_misc->width());
  Reg.setValue("w1", ui->frame->width());
  if (InfoWinShow)  //不显示不存储
  {
    Reg.setValue("h0", ui->frame->height());
    Reg.setValue("h1", ui->frameInfo->height());
  }

  //存储当前的目录结构
  QWidget* pWidget =
      ui->tabWidget_textEdit->widget(ui->tabWidget_textEdit->currentIndex());

  lblCurrentFile = (QLabel*)pWidget->children().at(
      lblNumber);  // 2为QLabel,1为textEdit,0为VBoxLayout

  QFileInfo f(lblCurrentFile->text());
  Reg.setValue("dir", f.path());
  Reg.setValue("btn", ui->btnReturn->text());
  Reg.setValue("ci",
               ui->tabWidget_textEdit->currentIndex());  //存储当前活动的标签页

  //检查文件是否修改，需要保存
  for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
    ui->tabWidget_textEdit->setCurrentIndex(i);
    pWidget = ui->tabWidget_textEdit->widget(i);
    textEdit = getCurrentEditor(i);

    lblCurrentFile = (QLabel*)pWidget->children().at(lblNumber);

    if (lblCurrentFile->text() == tr("untitled") + ".dsl")
      curFile = "";
    else
      curFile = lblCurrentFile->text();

    if (getCurrentEditor(i)->isModified()) {
      int choice;
      if (!zh_cn) {
        choice = QMessageBox::warning(
            this, tr("Application"),
            tr("The document has been modified.\n"
               "Do you want to save your changes?\n\n") +
                ui->tabWidget_textEdit->tabBar()->tabText(i),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

      } else {
        QMessageBox message(QMessageBox::Warning, "Xiasl",
                            "文件内容已修改，是否保存？\n\n" +
                                ui->tabWidget_textEdit->tabBar()->tabText(i));
        message.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                                   QMessageBox::Cancel);
        message.setButtonText(QMessageBox::Save, QString("保 存"));
        message.setButtonText(QMessageBox::Cancel, QString("取 消"));
        message.setButtonText(QMessageBox::Discard, QString("不保存"));
        message.setDefaultButton(QMessageBox::Save);
        choice = message.exec();
      }

      switch (choice) {
        case QMessageBox::Save:
          Save();
          event->accept();
          break;
        case QMessageBox::Discard:
          event->accept();
          break;
        case QMessageBox::Cancel:
          event->ignore();
          break;
      }
    } else {
      event->accept();
    }
  }

  // Save tabs
  int tabcount = ui->tabWidget_textEdit->tabBar()->count();
  int m = 0;
  for (int i = 0; i < tabcount; i++) {
    pWidget = ui->tabWidget_textEdit->widget(i);
    lblCurrentFile = (QLabel*)pWidget->children().at(lblNumber);
    if (lblCurrentFile->text() == tr("untitled") + ".dsl") {
    } else {
      getCurrentEditor(i)->getCursorPosition(&rowDrag, &colDrag);
      vs = getCurrentEditor(i)->verticalScrollBar()->sliderPosition();
      hs = getCurrentEditor(i)->horizontalScrollBar()->sliderPosition();

      Reg.setValue(QString::number(m) + "/" + "file", lblCurrentFile->text());
      Reg.setValue(QString::number(m) + "/" + "row", rowDrag);
      Reg.setValue(QString::number(m) + "/" + "col", colDrag);
      Reg.setValue(QString::number(m) + "/" + "vs", vs);
      Reg.setValue(QString::number(m) + "/" + "hs", hs);
      m++;
    }
  }
  Reg.setValue("count", m);

  // Save scribble board
  QSaveFile fileScribble(QDir::homePath() + "/.config/" + strAppName +
                         "/Scribble.txt");
  if (fileScribble.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream out(&fileScribble);
    out << ui->editScribble->document()->toPlainText();

    if (!fileScribble.commit()) {
      qDebug() << tr("Cannot write file %1:\n%2.")
                      .arg(QDir::toNativeSeparators(fileName));
    }
  }

  // In multi-window, close the form and delete yourself
  for (int i = 0; i < wdlist.count(); i++) {
    if (this == wdlist.at(i)) {
      wdlist.removeAt(i);
      filelist.removeAt(i);
    }
  }

  //关闭线程
  if (!thread_end) {
    break_run = true;
    mythread->quit();
    mythread->wait();
  }
}

void MainWindow::recentOpen(QString filename) {
  loadFile(openFile(filename), -1, -1);
}

void MainWindow::ssdtOpen(QString filename) {
  loadFile(openFile(filename), -1, -1);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
  if (e->mimeData()->hasFormat("text/uri-list")) {
    e->acceptProposedAction();
  }
}

void MainWindow::dropEvent(QDropEvent* e) {
  QList<QUrl> urls = e->mimeData()->urls();
  if (urls.isEmpty()) {
    return;
  }

  for (int i = 0; i < urls.count(); i++) {
    QString fileName = urls.at(i).toLocalFile();
    if (fileName.isEmpty()) {
      return;
    }

    loadFile(openFile(fileName), -1, -1);
  }
}

void MainWindow::init_statusBar() {
  locationLabel = new QLabel(this);
  statusBar()->addWidget(locationLabel);

  lblLayer = new QLabel(this);

  QFont lblFont;
  lblFont.setFamily(ui->treeWidget->font().family());
  lblLayer->setFont(lblFont);
  lblLayer->setAutoFillBackground(true);

  lblLayer->setText(tr(" Layer "));
  lblLayer->setTextInteractionFlags(
      Qt::TextSelectableByMouse);  //允许选择其中的文字

  editLayer = new QLineEdit();
  editLayer->setReadOnly(true);
  ui->statusbar->addPermanentWidget(lblLayer);

  lblEncoding = new QLabel(this);
  lblEncoding->setText("UTF-8");
  ui->statusbar->addPermanentWidget(lblEncoding);

  QComboBox* cboxEncoding = new QComboBox;
  cboxEncoding->addItem("UTF-8");
  cboxEncoding->addItem("GBK");
  ui->statusbar->layout()->setMargin(0);
  ui->statusbar->layout()->setSpacing(0);

  lblMsg = new QLabel(this);
  ui->statusbar->addPermanentWidget(lblMsg);
}

// Menu New
void MainWindow::newFile(QString file) {
  if (!thread_end) {
    break_run = true;  //通知打断线程
    mythread->quit();
    mythread->wait();
  }

  ui->treeWidget->clear();
  s_count = 0;
  d_count = 0;
  m_count = 0;
  QString lblMembers = "Scope(" + QString::number(s_count) + ")  " + "Device(" +
                       QString::number(d_count) + ")  " + "Method(" +
                       QString::number(m_count) + ")";
  ui->treeWidget->setHeaderLabel(lblMembers);
  ui->lblMembers->setText(lblMembers);

  // ui->tabWidget_misc->tabBar()->setTabText(0, lblMembers);

  ui->editShowMsg->clear();
  ui->editErrors->clear();
  ui->editWarnings->clear();
  ui->editRemarks->clear();

  myTextLexer = init_Lexer(file);
  init_Edit();
  // init_miniEdit();

  MyTabPage* page = new MyTabPage;

  QGridLayout* gridLayout = new QGridLayout(page);
  gridLayout->setMargin(0);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->layout()->setSpacing(0);
  QVBoxLayout* vboxLayout = new QVBoxLayout();
  vboxLayout->setMargin(2);
  vboxLayout->setContentsMargins(0, 0, 0, 0);
  vboxLayout->addWidget(textEdit);
  QLabel* lbl = new QLabel(tr("untitled") + ".dsl");
  vboxLayout->addWidget(lbl);
  lbl->setHidden(true);

  // QVBoxLayout* hboxLayout = new QVBoxLayout();
  hboxLayout = new QVBoxLayout();
  // hboxLayout->addWidget(miniEdit);
  hboxLayout->setMargin(0);
  hboxLayout->setContentsMargins(1, 0, 0, 0);
  hboxLayout->setSpacing(0);

  gridLayout->addLayout(vboxLayout, 0, 0);
  gridLayout->addLayout(hboxLayout, 0, 1);
  page->setLayout(gridLayout);

  ui->tabWidget_textEdit->addTab(page, tr("untitled") + ".dsl");

  ui->tabWidget_textEdit->setCurrentIndex(
      ui->tabWidget_textEdit->tabBar()->count() - 1);
  ui->tabWidget_textEdit->setTabsClosable(true);

  QIcon icon(":/icon/md0.svg");
  ui->tabWidget_textEdit->tabBar()->setTabIcon(
      ui->tabWidget_textEdit->tabBar()->count() - 1, icon);

  curFile = "";
  shownName = "";
  setWindowTitle(ver + tr("untitled") + ".dsl");

  textEdit->clear();
  textEditBack->clear();
  miniEdit->clear();
  init_ScrollBox();

  lblLayer->setText("");
  lblMsg->setText("");

  ui->treeWidget->setHidden(false);
}

QsciLexer* MainWindow::init_Lexer(QString file) {
  QsciLexer* myTextLexer;
  QString strSuffix;
  if (file != "")
    strSuffix = QFileInfo(file).suffix().toLower();
  else
    strSuffix = "";

  if (strSuffix == "" || strSuffix == "dsl" || strSuffix == "asl") {
    myTextLexer = new QscilexerCppAttach;
    return myTextLexer;
  }

  if (strSuffix == "py") {
    myTextLexer = new QsciLexerPython;
    return myTextLexer;
  }

  if (strSuffix == "c" || strSuffix == "cpp") {
    myTextLexer = new QsciLexerCPP;
    return myTextLexer;
  }

  if (strSuffix == "md") {
    myTextLexer = new QsciLexerMarkdown;
    return myTextLexer;
  }

  myTextLexer = new QscilexerCppAttach;

  return myTextLexer;
}

void MainWindow::on_btnReplaceFind() {
  on_btnReplace();
  if (find_down) {
    ui->btnSearch->clicked();
  }
  if (find_up) on_btnFindPrevious();
}

/*菜单：设置字体*/
void MainWindow::set_Font() {
  bool ok;
  QFontDialog fd;
  QFont font = get_Font();

  font = fd.getFont(&ok, font);

  if (ok) {
    for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
      myTextLexer->setFont(font);
      getCurrentEditor(i)->setLexer(myTextLexer);
      set_MyStyle(myTextLexer, getCurrentEditor(i));
    }

    ui->treeView->setFont(font);
    ui->treeWidget->setFont(font);

    //存储字体信息

    QSettings Reg(strIniFile, QSettings::IniFormat);
    Reg.setValue("FontName", font.family());
    Reg.setValue("FontSize", font.pointSize());
    Reg.setValue("FontBold", font.bold());
    Reg.setValue("FontItalic", font.italic());
    Reg.setValue("FontUnderline", font.underline());
  }
}

/*菜单：是否自动换行*/
void MainWindow::set_wrap() {
  if (dlgset->ui->cboxWrapWord->isChecked()) {
    textEdit->setWrapMode(QsciScintilla::WrapWord);
    miniEdit->setWrapMode(QsciScintilla::WrapWord);
    miniDlgEdit->setWrapMode(QsciScintilla::WrapWord);
  } else {
    textEdit->setWrapMode(QsciScintilla::WrapNone);
    miniEdit->setWrapMode(QsciScintilla::WrapNone);
    miniDlgEdit->setWrapMode(QsciScintilla::WrapNone);
  }
}

void MainWindow::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);

  //获取背景色,用来刷新软件使用中，系统切换亮、暗模式
  QPalette pal = ui->treeWidget->palette();
  QBrush brush = pal.window();
  int c_red = brush.color().red();
  if (c_red != red) {
    red = c_red;
    init_UIStyle();
    //注意：1.代码折叠线的颜色 2.双引号输入时的背景色

    for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
      QsciScintilla* edit = getCurrentEditor(i);
      QString file = getCurrentFileName(i);
      myTextLexer = init_Lexer(file);
      edit->setLexer(myTextLexer);
      set_MyStyle(myTextLexer, edit);

      miniLexer = init_Lexer(file);
      init_MiniText();
    }
  }
}

int MainWindow::treeCount(QTreeWidget* tree, QTreeWidgetItem* parent) {
  Q_ASSERT(tree != NULL);

  int count = 0;
  if (parent == 0) {
    int topCount = tree->topLevelItemCount();
    for (int i = 0; i < topCount; i++) {
      QTreeWidgetItem* item = tree->topLevelItem(i);
      if (item->isExpanded()) {
        count += treeCount(tree, item);
      }
    }
    count += topCount;
  } else {
    int childCount = parent->childCount();
    for (int i = 0; i < childCount; i++) {
      QTreeWidgetItem* item = parent->child(i);
      if (item->isExpanded()) {
        count += treeCount(tree, item);
      }
    }
    count += childCount;
  }
  return count;
}

int MainWindow::treeCount(QTreeWidget* tree) {
  Q_ASSERT(tree != NULL);

  int count = 0;

  int topCount = tree->topLevelItemCount();
  for (int i = 0; i < topCount; i++) {
    QTreeWidgetItem* item = tree->topLevelItem(i);
    if (item->isExpanded()) {
      count += treeCount(tree, item);
    }
  }
  count += topCount;

  return count;
}

/*获取当前条目的所有上级条目*/
QString MainWindow::getLayerName(QTreeWidgetItem* hItem) {
  if (!hItem) return "";

  QString str0 = hItem->text(0);  //记录初始值，即为当前被选中的条目值
  QString str;
  char sername[255];
  memset(sername, 0, 255);
  QVector<QString> list;
  QTreeWidgetItem* phItem = hItem->parent();  //获取当前item的父item
  if (!phItem) {                              // 根节点
    QString qstr = hItem->text(0);
    QByteArray ba = qstr.toLatin1();  //实现QString和 char *的转换
    const char* cstr = ba.data();
    strcpy(sername, cstr);
  } else {
    while (phItem) {
      QString qstr = phItem->text(0);
      QByteArray ba = qstr.toLatin1();  //实现QString和char *的转换
      const char* cstr = ba.data();
      strcpy(sername, cstr);
      phItem = phItem->parent();

      list.push_back(sername);
    }
  }
  for (int i = 0; i < list.count(); i++) {
    str = list.at(i) + " --> " + str;
  }

  return " " + str + str0 + " ";
}

void MainWindow::kextstat() {
  pk = new QProcess;
  QStringList cs1;
  pk->start("kextstat", cs1);
  connect(pk, SIGNAL(finished(int)), this, SLOT(readKextstat()));
}

void MainWindow::readKextstat() {
  QString result = QString::fromUtf8(pk->readAll());
  newFile("");
  textEdit->append(result);
  textEdit->setModified(false);

  ui->frameInfo->setHidden(true);
  ui->actionInfo_win->setChecked(false);
}

void MainWindow::loadLocal() {
  QTextCodec* codec = QTextCodec::codecForName("System");
  QTextCodec::setCodecForLocale(codec);

  static QTranslator translator;  //注意：使translator一直生效
  QLocale locale;
  if (locale.language() == QLocale::English)  //获取系统语言环境
  {
    zh_cn = false;

  } else if (locale.language() == QLocale::Chinese) {
    bool tr = false;
    tr = translator.load(":/tr/cn.qm");
    if (tr) {
      qApp->installTranslator(&translator);
      zh_cn = true;
    }

    ui->retranslateUi(this);
  }
}

void MainWindow::on_btnCompile() {
  miniDlg->close();

  btnCompile_clicked();
}

void MainWindow::on_treeView_doubleClicked(const QModelIndex& index) {
  fsm_Filepath = model->filePath(index);
  fsm_Index = index;

  if (!model->isDir(index)) {
    loadFile(openFile(fsm_Filepath), -1, -1);
  }
}

void MainWindow::on_btnReturn_clicked() {
  QString str = model->filePath(fsm_Index.parent());
  model->setRootPath(str);
  ui->treeView->setRootIndex(model->index(str));
  ui->btnReturn->setText(str);

  fsm_Index = fsm_Index.parent();
}

void MainWindow::on_treeView_expanded(const QModelIndex& index) {
  fsm_Index = index;
  QString str = model->filePath(index);
  set_return_text(str);
}

void MainWindow::on_treeView_collapsed(const QModelIndex& index) {
  fsm_Index = index;
  QString str = model->filePath(index);
  set_return_text(str);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched == ui->treeView) {
    if (event->type() == QEvent::FocusIn) {
    } else if (event->type() == QEvent::FocusOut) {
    }
  }

  if (watched == ui->treeWidget)  //判断控件
  {
    if (event->type() == QEvent::FocusIn)  //控件获得焦点事件)
    {
    } else if (event->type() == QEvent::FocusOut)  //控件失去焦点事件
    {
    }
  }

  //禁用鼠标滚轮切换标签页
  if (watched == ui->tabWidget_textEdit->tabBar()) {
    if (event->type() == QEvent::Wheel) {
      return true;
    }
  }

  if (watched == ui->fBox) {
    if (event->type() == QEvent::MouseMove) {
    }
  }

  if (watched == this) {
    if (event->type() == QEvent::ActivationChange) {
      if (QApplication::activeWindow() != this) {
      }
      if (QApplication::activeWindow() == this) {
      }
    }
  }

  if (watched == textEdit) {
    if (event->type() == QEvent::KeyPress) {
      if (m_searchTextPosList.count() > 0) {
        clearSearchHighlight(textEdit);
        m_searchTextPosList.clear();
        return true;
      }
    }
  }

  return QWidget::eventFilter(watched, event);
}

QString MainWindow::getCurrentFileName(int index) {
  QWidget* pWidget = ui->tabWidget_textEdit->widget(index);
  lblCurrentFile = (QLabel*)pWidget->children().at(lblNumber);
  return lblCurrentFile->text();
}

void MainWindow::on_tabWidget_textEdit_tabBarClicked(int index) {
  if (!thread_end) {
    break_run = true;

    mythread->quit();
    mythread->wait();

    /*等待线程结束,以使最后一次刷新可以完成*/
    while (!thread_end) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
  }

  miniDlg->close();

  if (index == -1)  //点击标签页之外的区域
    return;

  textEdit = getCurrentEditor(index);
  // miniEdit = getCurrentMiniEditor(index);

  if (miniEdit->isVisible())
    ui->actionMinimap->setChecked(true);
  else
    ui->actionMinimap->setChecked(false);

  //取消自动换行
  ui->actionAutomatic_Line_Feeds->setChecked(false);
  textEdit->setWrapMode(QsciScintilla::WrapNone);
  miniEdit->setWrapMode(QsciScintilla::WrapNone);
  miniDlgEdit->setWrapMode(QsciScintilla::WrapNone);

  QString currentFile = getCurrentFileName(index);
  if (currentFile == tr("untitled") + ".dsl") {
    curFile = "";

  } else {
    curFile = currentFile;
  }

  ui->treeWidget->clear();
  on_RefreshTree();

  QFileInfo f(curFile);
  if (f.suffix().toLower() == "dsl" || f.suffix().toLower() == "asl" ||
      f.suffix().toLower() == "cpp" || f.suffix().toLower() == "c") {
    ui->actionCompiling->setEnabled(true);
  } else {
    ui->actionCompiling->setEnabled(false);
  }

  setWindowTitle(ver + currentFile);

  //初始化fsm
  init_fsmSyncOpenedFile(curFile);

  getBookmarks();

  textEdit->setFocus();

  dragFileName = curFile;
  textEdit->getCursorPosition(&rowDrag, &colDrag);
  vs = textEdit->verticalScrollBar()->sliderPosition();
  hs = textEdit->horizontalScrollBar()->sliderPosition();
}

void MainWindow::init_fsmSyncOpenedFile(QString OpenedFile) {
  QFileInfo f(OpenedFile);
  fsm_Filepath = f.path();
  model->setRootPath(fsm_Filepath);
  fsm_Index = model->index(f.path());
  ui->treeView->setRootIndex(fsm_Index);
  set_return_text(fsm_Filepath);
  ui->treeView->setCurrentIndex(
      model->index(OpenedFile));  //设置当前条目为打开的文件

  ui->editFolder->setText(f.path());
}

QsciScintilla* MainWindow::getCurrentEditor(int index) {
  QWidget* pWidget = ui->tabWidget_textEdit->widget(index);
  QsciScintilla* edit;  // = new QsciScintilla;
  edit = (QsciScintilla*)pWidget->children().at(editNumber);

  return edit;
}

MiniEditor* MainWindow::getCurrentMiniEditor(int index) {
  QWidget* pWidget = ui->tabWidget_textEdit->widget(index);
  MiniEditor* edit;  // = new QsciScintilla;
  edit = (MiniEditor*)pWidget->children().at(3);

  return edit;
}

void MainWindow::closeTab(int index) {
  if (index < 0) return;

  if (ui->tabWidget_textEdit->tabBar()->count() > 1) {
    ui->tabWidget_textEdit->setCurrentIndex(index);
    textEdit = getCurrentEditor(index);

    QString currentFile = getCurrentFileName(index);

    if (currentFile == tr("untitled") + ".dsl")
      curFile = "";
    else
      curFile = currentFile;

    openFileList.removeOne(currentFile);
    if (curFile != "") FileSystemWatcher::removeWatchPath(currentFile);

    if (maybeSave(ui->tabWidget_textEdit->tabBar()->tabText(index))) {
      ui->tabWidget_textEdit->removeTab(index);
    }

  } else
    ui->tabWidget_textEdit->setTabsClosable(false);

  on_tabWidget_textEdit_tabBarClicked(ui->tabWidget_textEdit->currentIndex());
}

void MainWindow::on_tabWidget_textEdit_currentChanged(int index) {
  if (index < 0) return;
  if (ui->cboxFindScope->currentIndex() == 0) ui->treeFind->clear();
  index_treeFindChild = 0;

  if (index >= 0 && m_searchTextPosList.count() > 0) {
    for (int i = 0; i < ui->tabWidget_textEdit->tabBar()->count(); i++) {
      clearSearchHighlight(getCurrentEditor(i));
    }

    m_searchTextPosList.clear();
  }

  init_TabList();
}

void MainWindow::view_info() {
  if (ui->frameInfo->isHidden()) {
    ui->frameInfo->setHidden(false);
    InfoWinShow = true;
    ui->actionInfo_win->setChecked(true);
  } else if (!ui->frameInfo->isHidden()) {
    ui->frameInfo->setHidden(true);
    ui->actionInfo_win->setChecked(false);
  }

  init_ScrollBox();
}

void MainWindow::view_mem_list() {
  if (ui->tabWidget_misc->isHidden()) {
    ui->tabWidget_misc->setHidden(false);
    ui->actionMembers_win->setChecked(true);
  } else if (!ui->tabWidget_misc->isHidden()) {
    ui->tabWidget_misc->setHidden(true);
    ui->actionMembers_win->setChecked(false);
  }
}

void MainWindow::ds_Decompile() {
  dlg->setWindowTitle(tr("DSDT + SSDT Decompile"));
  dlg->setModal(true);
  dlg->show();
}

void MainWindow::iaslUsage() {
  pk = new QProcess;

  QFileInfo appInfo(qApp->applicationDirPath());

#ifdef Q_OS_WIN32
  pk->start(appInfo.filePath() + "/iasl.exe", QStringList() << "-h");
#endif

#ifdef Q_OS_LINUX
  pk->start(appInfo.filePath() + "/iasl", QStringList() << "-h");
#endif

#ifdef Q_OS_MAC
  pk->start(appInfo.filePath() + "/iasl", QStringList() << "-h");
#endif

  connect(pk, SIGNAL(finished(int)), this, SLOT(readHelpResult(int)));
}

void MainWindow::readHelpResult(int exitCode) {
  Q_UNUSED(exitCode);
  QString result;

  result = QString::fromUtf8(pk->readAll());
  newFile("");
  textEdit->append(result);
  textEdit->setModified(false);
}

void MainWindow::CheckUpdate() {
  QNetworkRequest quest;
  quest.setUrl(QUrl("https://api.github.com/repos/ic005k/" + strAppName +
                    "/releases/latest"));
  quest.setHeader(QNetworkRequest::UserAgentHeader, "RT-Thread ART");
  manager->get(quest);
}

// manager的槽函数
void MainWindow::replyFinished(QNetworkReply* reply) {
  QString str = reply->readAll();
  parse_UpdateJSON(str);
  reply->deleteLater();
}

QString MainWindow::getUrl(QVariantList list) {
  QString macUrl, winUrl, linuxUrl, osx1012Url;
  for (int i = 0; i < list.count(); i++) {
    QVariantMap map = list[i].toMap();
    QString fName = map["name"].toString();

    if (fName.contains("Xiasl_Mac.dmg"))
      macUrl = map["browser_download_url"].toString();

    if (fName.contains("Win")) winUrl = map["browser_download_url"].toString();

    if (fName.contains("Linux"))
      linuxUrl = map["browser_download_url"].toString();

    if (fName.contains("below"))
      osx1012Url = map["browser_download_url"].toString();
  }

  QString Url;
  if (mac) Url = macUrl;
  if (win) Url = winUrl;
  if (linuxOS) Url = linuxUrl;
  if (osx1012) Url = osx1012Url;

  return Url;
}

int MainWindow::parse_UpdateJSON(QString str) {
  QJsonParseError err_rpt;
  QJsonDocument root_Doc = QJsonDocument::fromJson(str.toUtf8(), &err_rpt);

  if (err_rpt.error != QJsonParseError::NoError) {
    if (!blAutoCheckUpdate)
      QMessageBox::critical(this, "", tr("Network error!"));
    blAutoCheckUpdate = false;
    return -1;
  }
  if (root_Doc.isObject()) {
    QJsonObject root_Obj = root_Doc.object();

    QVariantList list = root_Obj.value("assets").toArray().toVariantList();
    QString Url = getUrl(list);
    dlgAutoUpdate->strUrl = Url;
    QJsonObject PulseValue = root_Obj.value("assets").toObject();
    QString Verison = root_Obj.value("tag_name").toString();
    QString UpdateTime = root_Obj.value("published_at").toString();
    QString ReleaseNote = root_Obj.value("body").toString();

    if (Verison > CurVersion && Url != "") {
      QString warningStr = tr("New version detected!") + "\n" +
                           tr("Version: ") + "V" + Verison + "\n" +
                           tr("Published at: ") + UpdateTime + "\n" +
                           tr("Release Notes: ") + "\n" + ReleaseNote;
      int ret = QMessageBox::warning(this, "", warningStr, tr("Download"),
                                     tr("Cancel"));
      if (ret == 0) {
        ShowAutoUpdateDlg(false);
      }
    } else {
      if (!blAutoCheckUpdate)
        QMessageBox::information(
            this, "", tr("You are currently using the latest version!"));
    }
  }
  blAutoCheckUpdate = false;
  return 0;
}

void MainWindow::highlighsearchtext(QString searchText, QsciScintilla* textEdit,
                                    QString file, bool addTreeItem) {
  if (isBreakFind) {
    return;
  }

  if (searchText.trimmed() == "") return;

  std::string document;

  if (isCaseSensitive) {
    search_string = searchText;
    document = textEdit->text().toStdString();
  } else {
    search_string = searchText.toLower();
    document = textEdit->text().toLower().toStdString();
  }

  textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETSTYLE, 0,
                          QsciScintillaBase::INDIC_BOX);
  if (red < 55) {
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETOUTLINEALPHA, 0,
                            255);
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETALPHA, 0, 50);
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETFORE, 0,
                            QColor(Qt::white));
  } else {
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETOUTLINEALPHA, 0,
                            200);
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETALPHA, 0, 30);
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICSETFORE, 0,
                            QColor(Qt::red));
  }

  m_searchTextPosList.clear();

  //查找document中flag
  //出现的所有位置,采用标准字符串来计算，QString会有一些问题
  std::string flag = search_string.toStdString();

  unsigned long long position = 0;

  int i = 1;
  while ((position = document.find(flag, position)) != std::string::npos &&
         !isBreakFind) {
    textEdit->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, position,
                            search_string.toStdString().length());

    m_searchTextPosList.append(position);

    position++;
    i++;
  }

  curFindPos = m_searchTextPosList.count();
  totalPos = totalPos + curFindPos;

  if (file != curFile) {
    clearSearchHighlight(textEdit);
  }

  // 结果列表
  if (!addTreeItem) return;
  int count = m_searchTextPosList.count();
  if (count == 0) return;

  QTreeWidgetItem* topItem = new QTreeWidgetItem();
  topItem->setText(0, QString::number(count) + "    " + file);
  topItem->setText(1, file);

  for (int i = 0; i < m_searchTextPosList.count(); i++) {
    if (isBreakFind) {
      break;
      return;
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(topItem);
    unsigned long long pos = m_searchTextPosList.at(i);
    int row, col;
    row = textEdit->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, pos,
                                  NULL);
    col = textEdit->SendScintilla(QsciScintillaBase::SCI_GETCOLUMN, pos, NULL);

    QString lineText = textEdit->text(row).trimmed();
    item->setText(0, "( " + QString::number(row + 1) + " , " +
                         QString::number(col) + " )" + "    " + lineText);
    item->setText(1, QString::number(pos));
  }

  tw_SearchResults.append(topItem);
}

void MainWindow::clearSearchHighlight(QsciScintilla* textEdit) {
  for (int i = 0; i < m_searchTextPosList.count(); i++) {
    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICATORCLEARRANGE,
                            m_searchTextPosList[i],
                            search_string.toStdString().length());
  }
}

void MainWindow::on_editFind_editTextChanged(const QString& arg1) {
  if (AddCboxFindItem) return;

  ui->treeFind->clear();

  if (arg1.count() > 0) {
    return;
    on_btnSearch_clicked();

  } else {
    clearSearchHighlight(textEdit);
    ui->treeFind->clear();

    ui->editFind->lineEdit()->setPlaceholderText(tr("Find"));

    if (red < 55) {
      QPalette palette;
      palette = ui->editFind->palette();
      palette.setColor(QPalette::Base, QColor(50, 50, 50));
      palette.setColor(QPalette::Text, Qt::white);
      ui->editFind->setPalette(palette);

    } else {
      QPalette palette;
      palette = ui->editFind->palette();
      palette.setColor(QPalette::Base, Qt::white);
      palette.setColor(QPalette::Text, Qt::black);
      ui->editFind->setPalette(palette);
    }
  }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    if (ui->editFind->hasFocus()) {
    }
  }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
  if (ui->editFind->hasFocus()) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
      setEditFindCompleter();
    }
  }
}

void MainWindow::init_findTextList() {
  QString strBak = ui->editFind->currentText();

  for (int i = 0; i < findTextList.count(); i++) {
    if (ui->editFind->currentText().trimmed() == findTextList.at(i))

      findTextList.removeAt(i);
  }

  findTextList.insert(0, ui->editFind->currentText().trimmed());

  for (int i = 0; i < findTextList.count(); i++) {
    if (findTextList.at(i) == tr("Clear history entries"))

      findTextList.removeAt(i);
  }

  findTextList.append(tr("Clear history entries"));

  ui->editFind->clear();
  ui->editFind->addItems(findTextList);

  ui->editFind->setCurrentText(strBak);
}

void MainWindow::on_editFind_currentIndexChanged(const QString& arg1) {
  Q_UNUSED(arg1);
}

void MainWindow::on_clearFindText() {
  ui->editFind->clear();

  ui->editFind->lineEdit()->setPlaceholderText(tr("Find"));
}

QString MainWindow::getTabTitle() {
  int index = ui->tabWidget_textEdit->currentIndex();
  return ui->tabWidget_textEdit->tabBar()->tabText(index);
}

void MainWindow::on_NewWindow() {
  QFileInfo appInfo(qApp->applicationDirPath());
  QString pathSource = appInfo.filePath() + "/Xiasl";
  QStringList arguments;
  QString fn = "";
  arguments << fn;
  QProcess* process = new QProcess;
  process->setEnvironment(process->environment());
  process->start(pathSource, arguments);
}

void MainWindow::on_miniMap() {
  if (ui->tabWidget_textEdit->currentIndex() < 0) return;

  if (!ui->actionMinimap->isChecked()) {
    ui->dockMiniEdit->setVisible(false);
  } else {
    ui->dockMiniEdit->setVisible(true);
  }
}

void MiniEditor::mouseMoveEvent(QMouseEvent* event) {
  textEditScroll = false;
  miniEditWheel = false;
  mw_one->repaint();
  if (!textEditScroll) {
    showZoomWin(event->x(), event->y());
    if (mw_one->mac || mw_one->osx1012) {
      mw_one->update();
      mw_one->repaint();
    }
  }
}

// void MiniEditor::paintEvent(QPaintEvent*) {}

void MiniEditor::mousePressEvent(QMouseEvent* event) {
  mw_one->textEdit->setFocus();
  if (event->button() == Qt::LeftButton) {
    mw_one->textEdit->setCursorPosition(miniLineNum, 0);
  }

  if (event->button() == Qt::RightButton) {
    currentLineText = currentLineText.trimmed();
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(currentLineText);
  }
}

void MiniEditor::wheelEvent(QWheelEvent* event) {
  if (!this->isHidden()) return;

  int spos = this->verticalScrollBar()->sliderPosition();
  miniEditWheel = true;

  if (event->angleDelta().y() > 0) {
    this->verticalScrollBar()->setSliderPosition(spos - 3);

  } else {
    this->verticalScrollBar()->setSliderPosition(spos + 3);
  }
}

void MiniEditor::showZoomWin(int x, int y) {
  int totalLines = this->lines();

  if (x < 15) {
    miniDlg->close();
    return;
  }

  curY = y;
  int textHeight = this->textHeight(0);
  int y0 = y / textHeight + this->verticalScrollBar()->sliderPosition();

  QString t1, t2, t3, t4, t5, t6, t7, t8, t9;

  miniDlgEdit->clear();
  miniDlgEdit->append("\n\n\n");

  if (totalLines < 10) {
    currentLineText = this->text(4);
    for (int i = 0; i < totalLines; i++) {
      miniDlgEdit->append(QString::number(i + 1) + "  " + this->text(i));

      //清除所有标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL);

      // SCI_MARKERGET 参数用来设置标记，默认为圆形标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERGET, y0 + 3);

      // SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0,
                                 QColor(Qt::blue));
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0,
                                 QColor(Qt::blue));

      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, y0 + 3);
      //下划线
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_STYLESETUNDERLINE, y0 + 4,
                                 true);
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 0,
                                 QsciScintilla::SC_MARK_UNDERLINE);
    }
  } else {
    if (y0 <= 5) {
      t1 = this->text(0);
      t2 = this->text(1);
      t3 = this->text(2);
      t4 = this->text(3);
      t5 = this->text(4);
      t6 = this->text(5);
      t7 = this->text(6);
      t8 = this->text(7);
      t9 = this->text(8);

      currentLineText = t5;

      miniDlgEdit->append(QString::number(1) + "  " + t1);
      miniDlgEdit->append(QString::number(2) + "  " + t2);
      miniDlgEdit->append(QString::number(3) + "  " + t3);
      miniDlgEdit->append(QString::number(4) + "  " + t4);
      miniDlgEdit->append(QString::number(5) + "  " + t5);
      miniDlgEdit->append(QString::number(6) + "  " + t6);
      miniDlgEdit->append(QString::number(7) + "  " + t7);
      miniDlgEdit->append(QString::number(8) + "  " + t8);
      miniDlgEdit->append(QString::number(9) + "  " + t9);

      //清除所有标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL);

      // SCI_MARKERGET 参数用来设置标记，默认为圆形标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERGET, y0 + 3);

      // SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0,
                                 QColor(Qt::blue));
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0,
                                 QColor(Qt::blue));

      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, y0 + 3);
      //下划线
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_STYLESETUNDERLINE, y0 + 4,
                                 true);
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 0,
                                 QsciScintilla::SC_MARK_UNDERLINE);
    }

    if (y0 > 5 && y0 < totalLines - 9) {
      t1 = this->text(y0 - 4);
      t2 = this->text(y0 - 3);
      t3 = this->text(y0 - 2);
      t4 = this->text(y0 - 1);
      t5 = this->text(y0);
      t6 = this->text(y0 + 1);
      t7 = this->text(y0 + 2);
      t8 = this->text(y0 + 3);
      t9 = this->text(y0 + 4);

      currentLineText = t5;

      miniDlgEdit->append(QString::number(y0 - 3) + "  " + t1);
      miniDlgEdit->append(QString::number(y0 - 2) + "  " + t2);
      miniDlgEdit->append(QString::number(y0 - 1) + "  " + t3);
      miniDlgEdit->append(QString::number(y0 - 0) + "  " + t4);
      miniDlgEdit->append(QString::number(y0 + 1) + "  " + t5);
      miniDlgEdit->append(QString::number(y0 + 2) + "  " + t6);
      miniDlgEdit->append(QString::number(y0 + 3) + "  " + t7);
      miniDlgEdit->append(QString::number(y0 + 4) + "  " + t8);
      miniDlgEdit->append(QString::number(y0 + 5) + "  " + t9);

      //清除所有标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL);

      // SCI_MARKERGET 参数用来设置标记，默认为圆形标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERGET, 4);

      // SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0,
                                 QColor(Qt::blue));
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0,
                                 QColor(Qt::blue));

      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, 7);
      //下划线
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_STYLESETUNDERLINE, 8, true);
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 0,
                                 QsciScintilla::SC_MARK_UNDERLINE);
    }

    if (y0 >= totalLines - 9 && y0 <= totalLines) {
      t1 = this->text(totalLines - 9);
      t2 = this->text(totalLines - 8);
      t3 = this->text(totalLines - 7);
      t4 = this->text(totalLines - 6);
      t5 = this->text(totalLines - 5);
      t6 = this->text(totalLines - 4);
      t7 = this->text(totalLines - 3);
      t8 = this->text(totalLines - 2);
      t9 = this->text(totalLines - 1);

      currentLineText = t5;

      miniDlgEdit->append(QString::number(totalLines - 8) + "  " + t1);
      miniDlgEdit->append(QString::number(totalLines - 7) + "  " + t2);
      miniDlgEdit->append(QString::number(totalLines - 6) + "  " + t3);
      miniDlgEdit->append(QString::number(totalLines - 5) + "  " + t4);
      miniDlgEdit->append(QString::number(totalLines - 4) + "  " + t5);
      miniDlgEdit->append(QString::number(totalLines - 3) + "  " + t6);
      miniDlgEdit->append(QString::number(totalLines - 2) + "  " + t7);
      miniDlgEdit->append(QString::number(totalLines - 1) + "  " + t8);
      miniDlgEdit->append(QString::number(totalLines) + "  " + t9);

      //清除所有标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL);

      // SCI_MARKERGET 参数用来设置标记，默认为圆形标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERGET,
                                 9 - (totalLines - y0));

      // SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0,
                                 QColor(Qt::blue));
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0,
                                 QColor(Qt::blue));

      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERADD,
                                 9 - (totalLines - y0));
      //下划线
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_STYLESETUNDERLINE,
                                 9 - (totalLines - y0) + 1, true);
      miniDlgEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 0,
                                 QsciScintilla::SC_MARK_UNDERLINE);
    }
  }

  miniLineNum = y0;

  if (t1.trimmed() == "" && t5.trimmed() == "" && t2.trimmed() == "" &&
      t3.trimmed() == "" && t4.trimmed() == "") {
    miniDlg->close();
    return;
  }

  int w = 600;
  if (mw_one->ui->tabWidget_textEdit->width() > w)
    w = mw_one->textEdit->width() - 110;
  w = 350;
  if (mw_one->width() < w) w = mw_one->width() - width() - 2;

  int h = miniDlgEdit->textHeight(y) * 12 + 4;
  int y1 = y;

  if (y >= mw_one->getMainWindowHeight() - h)
    y1 = mw_one->getMainWindowHeight() - h;
  else
    y1 = y;

  int w0 = 0;
  if (mw_one->mac || mw_one->osx1012)
    w0 = 6;
  else
    w0 = 2;

  miniDlg->setGeometry(mw_one->ui->dockMiniEdit->x() - w - 1, y1, w, h);

  if (miniDlg->isHidden()) {
    QFont font = mw_one->get_Font();
    font.setPointSizeF(10);
    miniDlgEdit->setFont(font);
    miniDlg->show();
  }
}

void MiniEditor::miniEdit_cursorPositionChanged() {}

void MiniEditor::miniEdit_verticalScrollBarChanged() {
  miniDlg->close();

  if (loading) return;

  textEditScroll = false;
  mw_one->setValue();

  return;

  if (!textEditScroll) {
    if (curY == 0) curY = this->height() / 2;
    int p = verticalScrollBar()->sliderPosition();
    if (p > p0)
      curY = height();
    else
      curY = 0;

    showZoomWin(20, curY);

  } else
    miniDlg->close();
}

bool MiniEditor::eventFilter(QObject* watched, QEvent* event) {
  return QWidget::eventFilter(watched, event);
  if (watched == this->verticalScrollBar()) {
    if (event->type() == QEvent::MouseButtonRelease) {
      mw_one->textEdit->setFocus();
      mw_one->textEdit->setCursorPosition(miniLineNum, 0);

      p0 = verticalScrollBar()->sliderPosition();
      return true;
    }
  }

  if (watched == this) {
    if (event->type() == QEvent::ActivationChange) {
      if (QApplication::activeWindow() != this) {
      }
      if (QApplication::activeWindow() == this) {
      }
    }
  }

  return QWidget::eventFilter(watched, event);
}

void MaxEditor::mouseMoveEvent(QMouseEvent* event) {
  Q_UNUSED(event);

  miniDlg->close();
}

void MainWindow::setValue() {
  // miniEdit滚动条更改事件

  int t = textEdit->verticalScrollBar()->maximum();
  int m = miniEdit->verticalScrollBar()->maximum();
  double b =
      (double)miniEdit->verticalScrollBar()->sliderPosition() / (double)m;
  int p = b * t;

  textEdit->verticalScrollBar()->setSliderPosition(p);
}

void MainWindow::setValue2() {
  // textEdit滚动条更改事件

  if (miniEdit->isHidden()) return;

  if (!textEditScroll) {
    textEditScroll = true;
    return;
  }

  int t = textEdit->verticalScrollBar()->maximum();
  int m = miniEdit->verticalScrollBar()->maximum();
  double b =
      (double)textEdit->verticalScrollBar()->sliderPosition() / (double)t;
  int p = b * m;

  miniEdit->verticalScrollBar()->setSliderPosition(p);

  init_ScrollBox();
}

void MainWindow::init_ScrollBox() {
  int y0 = 0;

  int miniHeight, th;
  miniHeight = miniEdit->height();
  th = miniEdit->textHeight(1) * miniEdit->lines();
  if (th <= miniEdit->height()) {
    miniHeight = th;
  }

  int h0 = miniHeight - ui->fBox->height();

  int h1 = textEdit->verticalScrollBar()->maximum();
  int p1 = textEdit->verticalScrollBar()->sliderPosition();
  double b = (double)(p1) / (double)(h1);
  int p0 = h0 * b;
  int y = p0 + y0;

  miniEdit->setGeometry(miniEdit->x(), 0, ui->dockMiniEdit->width() - 1,
                        ui->dockMiniEdit->height());
  ui->fBox->setGeometry(miniEdit->x(), y, miniEdit->width() - 1, 30);
  ui->fBox->raise();
  ui->fBox->show();
}

#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent* event) { Q_UNUSED(event); }
#endif

void MainWindow::on_actionOpenDir() {
  QString dir = "file:" + model->filePath(fsm_Index);
  QDesktopServices::openUrl(QUrl(dir, QUrl::TolerantMode));
}

void MainWindow::on_actionExpandAll() { ui->treeWidget->expandAll(); }

void MainWindow::on_actionCollapseAll() { ui->treeWidget->collapseAll(); }

void MainWindow::moveEvent(QMoveEvent* e) { Q_UNUSED(e); }

bool MainWindow::event(QEvent* event) {
  if (event->type() == QEvent::NonClientAreaMouseButtonPress) {
    // qDebug() << "title clicked event";
  }

  if (event->type() == QEvent::NonClientAreaMouseButtonRelease) {
  }

  return QWidget::event(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* e) {
  if (!isDrag) return;

  miniDlg->close();

  if (!isDrag & (e->buttons() & Qt::LeftButton)) {
    move(e->globalPos() - m_position);
    e->accept();
  }

  e->accept();

  int miniHeight = miniEdit->height();

  int y0, y, y1, my;
  y0 = 0;
  y1 = y0 + miniHeight - ui->fBox->height();

  int th = miniEdit->textHeight(1) * miniEdit->lines();
  if (th <= miniEdit->height()) {
    int th = miniEdit->textHeight(1) * miniEdit->lines();
    int nh = th - ui->fBox->height();
    y1 = y0 + nh;
  }

  y = e->y();

  if (y <= y0) {
    my = y0;
  } else if (y >= y1) {
    my = y1;
  } else
    my = y - ui->fBox->height();

  if (my <= y0) my = y0;

  ui->fBox->setGeometry(miniEdit->x(), my, miniEdit->width() - 1, 30);
  ui->fBox->show();

  int t = miniHeight - ui->fBox->height();

  unsigned long max = miniEdit->verticalScrollBar()->maximum();
  int thisP = my - y0;
  double b = (double)(thisP) / (double)t;
  unsigned long p = b * max;

  miniEdit->verticalScrollBar()->setSliderPosition(p);

  if (th <= miniEdit->height()) {
    t = th - ui->fBox->height();
    max = textEdit->verticalScrollBar()->maximum();
    double b = (double)(thisP) / (double)t;
    p = b * max;
    textEdit->verticalScrollBar()->setSliderPosition(p);
  }
}

void MainWindow::mousePressEvent(QMouseEvent* e) {
  if (e->x() < ui->dockMiniEdit->x() ||
      e->x() > ui->dockMiniEdit->x() + ui->dockMiniEdit->width() - 5) {
    e->accept();
    return;
  }

  if (e->button() == Qt::LeftButton) {
    isDrag = true;
    m_position = e->globalPos() - this->pos();
    e->accept();
  }
}

void MainWindow::mouseReleaseEvent(QMouseEvent*) { isDrag = false; }

bool MainWindow::enterEdit(QPoint pp, QsciScintilla* btn) {
  int height = btn->height();
  int width = btn->width();
  QPoint btnMinPos = btn->pos();
  QPoint btnMaxPos = btn->pos();
  btnMaxPos.setX(btn->pos().x() + width);
  btnMaxPos.setY(btn->pos().y() + height);
  if (pp.x() >= btnMinPos.x() && pp.y() >= btnMinPos.y() &&
      pp.x() <= btnMaxPos.x() && pp.y() <= btnMaxPos.y())
    return true;
  else
    return false;
}

int MainWindow::getDockWidth() {
  if (ui->tabWidget_misc->x() != 0) return 0;

  if (ui->tabWidget_misc->isVisible())
    return ui->tabWidget_misc->width();
  else
    return 0;
}

int MainWindow::getMainWindowHeight() { return this->height(); }

int MainWindow::getMiniDockX() { return miniEdit->x(); }

int MainWindow::getTabWidgetEditX() { return ui->tabWidget_textEdit->x(); }

int MainWindow::getTabWidgetEditW() {
  return ui->tabWidget_textEdit->width() + 6;
}

void MainWindow::on_PreviousError() {
  if (QFileInfo(curFile).suffix().toLower() == "dsl") on_btnPreviousError();

  if (QFileInfo(curFile).suffix().toLower() == "cpp" ||
      QFileInfo(curFile).suffix().toLower() == "c")
    goCppPreviousError();
}

void MainWindow::on_NextError() {
  if (QFileInfo(curFile).suffix().toLower() == "dsl") on_btnNextError();

  if (QFileInfo(curFile).suffix().toLower() == "cpp" ||
      QFileInfo(curFile).suffix().toLower() == "c")
    goCppNextError();
}

void MainWindow::msg(int value) {
  QMessageBox box;
  box.setText(QString::number(value));
  box.exec();
}

void MainWindow::msgstr(QString str) {
  QMessageBox box;
  box.setText(str);
  box.exec();
}

void MainWindow::setEditFindCompleter() {
  QStringList valueList;
  for (int i = 0; i < ui->editFind->count(); i++) {
    valueList.append(ui->editFind->itemText(i));
  }
  QCompleter* editFindCompleter = new QCompleter(valueList, this);
  editFindCompleter->setCaseSensitivity(Qt::CaseSensitive);
  editFindCompleter->setCompletionMode(QCompleter::InlineCompletion);
  ui->editFind->setCompleter(editFindCompleter);
}

void MainWindow::on_listWidget_itemSelectionChanged() {
  int index = ui->listWidget->currentRow();
  ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::on_tabWidget_misc_currentChanged(int index) {
  Q_UNUSED(index);
}

void MainWindow::loadFindString() {
  //读取之前的目录

  QFileInfo fi(iniFile);

  if (fi.exists()) {
    QSettings Reg(iniFile, QSettings::IniFormat);

    //读取搜索文本
    int count = Reg.value("countFindText").toInt();
    for (int i = 0; i < count; i++) {
      QString item = Reg.value("FindText" + QString::number(i + 1)).toString();

      findTextList.append(item);
    }
    ui->editFind->addItems(findTextList);

    ui->editFind->setCurrentText("");
  }
}

void MainWindow::on_editFind_currentTextChanged(const QString& arg1) {
  Q_UNUSED(arg1);
}

void MainWindow::on_actionQuit_triggered() { this->close(); }

void MainWindow::on_actionClose_tab_triggered() {
  int count = ui->tabWidget_textEdit->tabBar()->count();
  int index = ui->tabWidget_textEdit->currentIndex();
  if (count > 1)
    ui->tabWidget_textEdit->tabBar()->tabCloseRequested(index);
  else if (count == 1)
    this->close();
}

void MainWindow::on_btnNo_clicked() {
  ui->frameTip->setHidden(true);
  reLoadByModiList.removeOne(strModiFile);

  checkReloadFilesByModi();
}

void MainWindow::on_btnYes_clicked() {
  loadFile(strModiFile, -1, -1);
  ui->frameTip->setHidden(true);
  reLoadByModiList.removeOne(strModiFile);

  checkReloadFilesByModi();
}

void MainWindow::checkReloadFilesByModi() {
  if (reLoadByModiList.count() > 0) {
    strModiFile = reLoadByModiList.at(0);
    ui->lblFileName->setText(tr("The file has been modified by another "
                                "program. Do you want to reload?") +
                             "\n\n" + QString("%1").arg(strModiFile));
    ui->frameTip->setHidden(false);
  }
}

void MainWindow::on_actionDownload_Upgrade_Packages_triggered() {
  ShowAutoUpdateDlg(false);
}

void MainWindow::ShowAutoUpdateDlg(bool Database) {
  if (dlgAutoUpdate->strUrl == "") return;
  if (dlgAutoUpdate->isVisible()) return;

  // dlgAutoUpdate->setWindowFlags(dlgAutoUpdate->windowFlags() |
  //                              Qt::WindowStaysOnTopHint);
  dlgAutoUpdate->setModal(true);
  dlgAutoUpdate->show();
  dlgAutoUpdate->startDownload(Database);
}

void MainWindow::on_actProxy1_triggered() {
  ui->actProxy1->setChecked(true);
  ui->actProxy2->setChecked(false);
  ui->actProxy3->setChecked(false);
  ui->actProxy4->setChecked(false);
  ui->actProxy5->setChecked(false);
  writeINIProxy();
}

void MainWindow::on_actProxy2_triggered() {
  ui->actProxy1->setChecked(false);
  ui->actProxy2->setChecked(true);
  ui->actProxy3->setChecked(false);
  ui->actProxy4->setChecked(false);
  ui->actProxy5->setChecked(false);
  writeINIProxy();
}

void MainWindow::on_actProxy3_triggered() {
  ui->actProxy1->setChecked(false);
  ui->actProxy2->setChecked(false);
  ui->actProxy3->setChecked(true);
  ui->actProxy4->setChecked(false);
  ui->actProxy5->setChecked(false);
  writeINIProxy();
}

void MainWindow::on_actProxy4_triggered() {
  ui->actProxy1->setChecked(false);
  ui->actProxy2->setChecked(false);
  ui->actProxy3->setChecked(false);
  ui->actProxy4->setChecked(true);
  ui->actProxy5->setChecked(false);
  writeINIProxy();
}

void MainWindow::on_actProxy5_triggered() {
  ui->actProxy1->setChecked(false);
  ui->actProxy2->setChecked(false);
  ui->actProxy3->setChecked(false);
  ui->actProxy4->setChecked(false);
  ui->actProxy5->setChecked(true);
  writeINIProxy();
}

void MainWindow::writeINIProxy() {
  QSettings Reg(strIniFile, QSettings::IniFormat);
  Reg.setValue("proxy1", ui->actProxy1->isChecked());
  Reg.setValue("proxy2", ui->actProxy2->isChecked());
  Reg.setValue("proxy3", ui->actProxy3->isChecked());
  Reg.setValue("proxy4", ui->actProxy4->isChecked());
  Reg.setValue("proxy5", ui->actProxy5->isChecked());
}

void MainWindow::readINIProxy() {
  QSettings Reg(strIniFile, QSettings::IniFormat);

  if (!Reg.allKeys().contains("proxy1")) {
    QLocale locale;
    if (locale.language() != QLocale::Chinese) {
      ui->actProxy1->setChecked(Reg.value("proxy1", true).toBool());
      on_actProxy1_triggered();
    }
  } else
    ui->actProxy1->setChecked(Reg.value("proxy1").toBool());

  if (!Reg.allKeys().contains("proxy3")) {
    QLocale locale;
    if (locale.language() == QLocale::Chinese) {
      ui->actProxy3->setChecked(Reg.value("proxy3", true).toBool());
      on_actProxy3_triggered();
    }
  } else
    ui->actProxy3->setChecked(Reg.value("proxy3").toBool());

  ui->actProxy2->setChecked(Reg.value("proxy2").toBool());
  ui->actProxy4->setChecked(Reg.value("proxy4").toBool());
  ui->actProxy5->setChecked(Reg.value("proxy5").toBool());
}

QString MainWindow::getProxy() {
  if (ui->actProxy1->isChecked()) return ui->actProxy1->text();
  if (ui->actProxy2->isChecked()) return ui->actProxy2->text();
  if (ui->actProxy3->isChecked()) return ui->actProxy3->text();
  if (ui->actProxy4->isChecked()) return ui->actProxy4->text();
  if (ui->actProxy5->isChecked()) return ui->actProxy5->text();

  return "";
}

void MainWindow::on_actionPreferences_triggered() {
  dlgset->setModal(true);
  dlgset->show();
}

void MainWindow::on_btnNext_clicked() {
  if (ui->treeFind->topLevelItemCount() == 0) return;
  int count = ui->treeFind->topLevelItemCount();
  int topIndex = 0;
  for (int i = 0; i < count; i++) {
    QString file = ui->treeFind->topLevelItem(i)->text(1);

    if (file == curFile) {
      topIndex = i;
      break;
    }
  }

  int childCount = ui->treeFind->topLevelItem(topIndex)->childCount();
  int row0, col0, row, col;
  textEdit->getCursorPosition(&row0, &col0);

  unsigned long long pos = ui->treeFind->topLevelItem(topIndex)
                               ->child(index_treeFindChild)
                               ->text(1)
                               .toLongLong();

  row = textEdit->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, pos,
                                NULL);
  col = textEdit->SendScintilla(QsciScintillaBase::SCI_GETCOLUMN, pos, NULL);

  if (row != row0 && col != col0 - findStr.length()) {
    for (int i = 0; i < childCount; i++) {
      pos =
          ui->treeFind->topLevelItem(topIndex)->child(i)->text(1).toLongLong();

      row = textEdit->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION,
                                    pos, NULL);
      col =
          textEdit->SendScintilla(QsciScintillaBase::SCI_GETCOLUMN, pos, NULL);

      if (row == row0 && col == col0 - findStr.length()) {
        index_treeFindChild = i;
        break;
      }
    }
  }
  ui->treeFind->setCurrentItem(
      ui->treeFind->topLevelItem(topIndex)->child(index_treeFindChild));

  index_treeFindChild = ui->treeFind->currentIndex().row() + 1;

  if (index_treeFindChild >= childCount) {
    index_treeFindChild = 0;
  }

  ui->treeFind->setCurrentItem(
      ui->treeFind->topLevelItem(topIndex)->child(index_treeFindChild));

  on_treeFind_itemClicked(
      ui->treeFind->topLevelItem(topIndex)->child(index_treeFindChild), 0);
}

void MainWindow::on_btnPrevious_clicked() {
  if (ui->treeFind->topLevelItemCount() == 0) return;
  int count = ui->treeFind->topLevelItemCount();
  int topIndex = 0;
  for (int i = 0; i < count; i++) {
    QString file = ui->treeFind->topLevelItem(i)->text(1);

    if (file == curFile) {
      topIndex = i;
      break;
    }
  }

  index_treeFindChild = ui->treeFind->currentIndex().row() - 1;

  if (index_treeFindChild < 0) {
    index_treeFindChild = 0;
  }

  ui->treeFind->setCurrentItem(
      ui->treeFind->topLevelItem(topIndex)->child(index_treeFindChild));
  on_treeFind_itemClicked(
      ui->treeFind->topLevelItem(topIndex)->child(index_treeFindChild), 0);
}

void MainWindow::on_btnDone_clicked() {}

void MainWindow::on_btnReplace_clicked() { on_btnReplace(); }

void MainWindow::on_btnReplaceFind_clicked() { on_btnReplaceFind(); }

void MainWindow::on_btnReplaceAll_clicked() { ReplaceAll(); }

void MainWindow::on_btnFind_clicked() { ui->btnSearch->clicked(); }

void MainWindow::on_btnCompile_clicked() { on_btnCompile(); }

void MainWindow::on_btnErrorP_clicked() { on_btnPreviousError(); }

void MainWindow::on_btnErrorN_clicked() { on_btnNextError(); }

void MainWindow::on_btnCaseSensitive_clicked() {
  QFont font;

  if (!isCaseSensitive) {
    isCaseSensitive = true;
    font.setBold(true);
    ui->btnCaseSensitive->setFont(font);
  } else {
    isCaseSensitive = false;
    font.setBold(false);
    ui->btnCaseSensitive->setFont(font);
  }

  on_btnSearch_clicked();
}

void MainWindow::on_btnSave_clicked() { Save(); }

void MainWindow::on_btnNew_clicked() { newFile(""); }

QString MainWindow::getMD5(QString targetFile) {
  QCryptographicHash hashTest(QCryptographicHash::Md5);
  QFile f2(targetFile);
  f2.open(QFile::ReadOnly);
  hashTest.reset();  // 重置（很重要）
  hashTest.addData(&f2);
  QString targetHash = hashTest.result().toHex();
  f2.close();
  return targetHash;
}

void MainWindow::updateMd5(QString file) {
  for (int i = 0; i < listMd5.count(); i++) {
    QString str = listMd5.at(i);
    QStringList list = str.split("|");
    if (list.count() == 2) {
      if (file == list.at(0)) {
        listMd5.removeAt(i);
      }
    }
  }

  listMd5.append(file + "|" + getMD5(file));
}

QString MainWindow::getMD5FromList(QString file) {
  for (int i = 0; i < listMd5.count(); i++) {
    QString str = listMd5.at(i);
    QStringList list = str.split("|");
    if (list.count() == 2) {
      if (file == list.at(0)) {
        return list.at(1);
      }
    }
  }

  return "";
}

void MainWindow::on_btnShowRepace_clicked() {}

void MainWindow::on_actionReporting_Issues_triggered() {
  QUrl url(QString("https://github.com/ic005k/" + strAppName + "/issues"));
  QDesktopServices::openUrl(url);
}

void MainWindow::on_actionUser_Guide_triggered() {
  QUrl url(QString("https://acpica.org/documentation"));
  QDesktopServices::openUrl(url);
}

void MainWindow::on_actionLatest_Release_triggered() {
  QUrl url(
      QString("https://github.com/ic005k/" + strAppName + "/releases/latest"));
  QDesktopServices::openUrl(url);
}

void MainWindow::init_UIStyle() {
  if (mac || osx1012) {
    this->setUnifiedTitleAndToolBarOnMac(false);
    if (red < 55) {
      if (unifiedTitleAndToolBarOnMac())
        this->setStyleSheet("QMainWindow { background-color: rgb(42,42,42);}");
      ui->tabWidget_misc->setStyleSheet(tabStyleDark);
      ui->tabWidget_textEdit->setStyleSheet(tabStyleDark);
      ui->statusbar->setStyleSheet(sbarStyleDark);

    } else {
      if (unifiedTitleAndToolBarOnMac())
        this->setStyleSheet("QMainWindow { background-color:rgb(212,212,212);");
      ui->tabWidget_misc->setStyleSheet(tabStyleLight);
      ui->tabWidget_textEdit->setStyleSheet(tabStyleLight);
      ui->statusbar->setStyleSheet(sbarStyleLight);
    }
  }

  if (win || linuxOS) {
    QString tabBarStyle = "QTabBar::tab{min-height:35px;}";
    ui->tabWidget_misc->setStyleSheet(tabBarStyle);
    ui->tabWidget_textEdit->setStyleSheet(tabBarStyle);
  }

  if (red > 55) {
    ui->treeWidget->setStyleSheet(treeWidgetStyleLight);
    ui->treeFind->setStyleSheet(treeFindStyleLight);
    ui->listWidget->setStyleSheet(infoShowStyleLight);
    ui->treeView->setStyleSheet(treeViewStyleLight);

    QPalette label_palette;
    label_palette.setColor(QPalette::Background, QColor(180, 209, 255));
    label_palette.setColor(QPalette::WindowText, Qt::black);
    lblLayer->setPalette(label_palette);
  } else {
    ui->treeWidget->setStyleSheet(treeWidgetStyleDark);
    ui->treeFind->setStyleSheet(treeFindStyleDark);
    ui->listWidget->setStyleSheet(infoShowStyleDark);
    ui->treeView->setStyleSheet(treeViewStyleDark);

    QPalette label_palette;
    label_palette.setColor(QPalette::Background, QColor(66, 92, 141));
    label_palette.setColor(QPalette::WindowText, QColor(226, 230, 237));
    lblLayer->setPalette(label_palette);
  }

  // miniDlgEdit
  QColor colorB, colorF;

  if (red > 55) {
    colorB.setRgb(245, 245, 245, 255);
    colorF.setRgb(0, 0, 0, 255);
  } else {
    colorB.setRgb(50, 50, 50, 255);
    colorF.setRgb(245, 245, 245, 255);
  }

  miniDlgEdit->setPaper(colorB);
  miniDlgEdit->setColor(colorF);
}

void MainWindow::on_btnTabList_clicked() {}

void MainWindow::init_TabList() {
  if (ui->tabWidget_textEdit->count() <= 0) return;

  QList<QAction*> actList;
  int ci = ui->tabWidget_textEdit->currentIndex();
  for (int i = 0; i < ui->tabWidget_textEdit->count(); i++) {
    QString txt = ui->tabWidget_textEdit->tabText(i);
    QAction* act = new QAction(this);
    act->setCheckable(true);
    if (i == ci) act->setChecked(true);
    act->setText(QString::number(i + 1) + " . " + txt);
    connect(act, &QAction::triggered, [=]() {
      ui->tabWidget_textEdit->setCurrentIndex(i);
      on_tabWidget_textEdit_tabBarClicked(i);
    });

    actList.append(act);
  }

  if (actList.count() > 0) {
    menuTabList->clear();
    menuTabList->addActions(actList);
  }
}

void MainWindow::changeEvent(QEvent* e) {
  Q_UNUSED(e);

#ifdef __APPLE__
  return;
  OSXHideTitleBar::HideTitleBar(winId());
#endif
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  Q_UNUSED(event);
  isDrag = false;
  miniDlg->close();

  miniEdit->setGeometry(miniEdit->x(), 0, ui->dockMiniEdit->width() - 1,
                        ui->dockMiniEdit->height());

  init_ScrollBox();
}

void MainWindow::on_actionAutomatic_Line_Feeds_triggered() {
  if (ui->actionAutomatic_Line_Feeds->isChecked()) {
    textEdit->setWrapMode(QsciScintilla::WrapWord);
    miniEdit->setWrapMode(QsciScintilla::WrapWord);
    miniDlgEdit->setWrapMode(QsciScintilla::WrapWord);
  } else {
    textEdit->setWrapMode(QsciScintilla::WrapNone);
    miniEdit->setWrapMode(QsciScintilla::WrapNone);
    miniDlgEdit->setWrapMode(QsciScintilla::WrapNone);
  }
}

void MainWindow::on_btnMiniMap_clicked() {
  miniDlg->close();
  if (miniEdit->isHidden()) {
    miniEdit->setHidden(false);
    textEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, false);
    textEditScroll = true;
    setValue2();
  } else {
    miniEdit->setHidden(true);
    textEdit->SendScintilla(QsciScintilla::SCI_SETVSCROLLBAR, true);
  }
}

void MainWindow::on_actionNew_triggered() { newFile(""); }

void MainWindow::on_treeFind_itemClicked(QTreeWidgetItem* item, int column) {
  if (column == 0) {
    if (item->childCount() > 0) return;

    if (ui->cboxFindScope->currentIndex() == 1) {
      QString file = item->parent()->text(1);
      if (file != curFile) {
        for (int i = 0; i < ui->tabWidget_textEdit->count(); i++) {
          if (file == getCurrentFileName(i)) {
            ui->tabWidget_textEdit->setCurrentIndex(i);
            on_tabWidget_textEdit_tabBarClicked(i);

            break;
          }
        }
      }
    }

    if (ui->cboxFindScope->currentIndex() == 2) {
      QString file = item->parent()->text(1);
      bool isOpen = false;
      if (file != curFile) {
        for (int i = 0; i < ui->tabWidget_textEdit->count(); i++) {
          if (file == getCurrentFileName(i)) {
            ui->tabWidget_textEdit->setCurrentIndex(i);
            on_tabWidget_textEdit_tabBarClicked(i);

            isOpen = true;
            break;
          }
        }

        if (!isOpen) {
          loadFile(file, -1, -1);
        }
      }
    }

    highlighsearchtext(findStr, textEdit, curFile, false);

    textEdit->setFocus();
    unsigned long long pos = item->text(1).toLongLong();
    int row, col;
    row = textEdit->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, pos,
                                  NULL);
    col = textEdit->SendScintilla(QsciScintillaBase::SCI_GETCOLUMN, pos, NULL);
    textEdit->setCursorPosition(row, col);

    if (textEdit->findFirst(findStr, true, isCaseSensitive, false, true,
                            true)) {
      if (red < 55) {
        QPalette palette;
        palette.setColor(QPalette::Text, Qt::white);
        ui->editFind->setPalette(palette);

        palette = ui->editFind->palette();
        palette.setColor(QPalette::Base, QColor(50, 50, 50, 255));
        ui->editFind->setPalette(palette);

      } else {
        QPalette palette;
        palette.setColor(QPalette::Text, Qt::black);
        ui->editFind->setPalette(palette);

        palette = ui->editFind->palette();
        palette.setColor(QPalette::Base, Qt::white);
        ui->editFind->setPalette(palette);
      }

    } else {
    }

    // qDebug() << row << col;

    init_ScrollBox();
  }
}

void MainWindow::on_btnSearch_clicked() {
  clearSearchHighlight(textEdit);
  ui->treeFind->clear();
  tw_SearchResults.clear();
  findStr = ui->editFind->currentText();
  index_treeFindChild = 0;

  if (ui->cboxFindScope->currentIndex() == 0) {
    int index = ui->tabWidget_textEdit->currentIndex();
    on_StartSearch(getCurrentEditor(index), getCurrentFileName(index));
    ui->treeFind->addTopLevelItems(tw_SearchResults);
  }

  if (ui->cboxFindScope->currentIndex() == 1) {
    int count = ui->tabWidget_textEdit->count();
    for (int i = 0; i < count; i++) {
      clearSearchHighlight(textEdit);
      on_StartSearch(getCurrentEditor(i), getCurrentFileName(i));
    }

    ui->treeFind->addTopLevelItems(tw_SearchResults);
  }

  if (ui->cboxFindScope->currentIndex() == 2) {
    on_btnStopFind_clicked();
    if (!isFinishFind) return;

    if (findStr == "") return;

    ui->progressBar->setMaximum(0);
    ui->lblSearch->setText(tr("Files") + " : 0    " + tr("Results") + " : 0" +
                           "    " + tr("Matches") + " : 0");
    findPath = ui->editFolder->text().trimmed();

    isFinishFind = false;
    isBreakFind = false;
    curFindPos = 0;
    totalPos = 0;

    mySearchThread->start();
    tmeShowFindProgress->start(100);
  }

  if (ui->cboxFindScope->currentIndex() != 2) setEditFindMarker();
}

void MainWindow::setEditFindMarker() {
  if (findStr.count() > 0) {
    if (tw_SearchResults.count() == 0) {
      QPalette palette;
      palette.setColor(QPalette::Text, Qt::white);
      ui->editFind->setPalette(palette);

      palette = ui->editFind->palette();
      palette.setColor(QPalette::Base, QColor(255, 70, 70));
      ui->editFind->setPalette(palette);
    } else {
      if (red < 55) {
        QPalette palette;
        palette = ui->editFind->palette();
        palette.setColor(QPalette::Base, QColor(50, 50, 50));
        palette.setColor(QPalette::Text, Qt::white);
        ui->editFind->setPalette(palette);

      } else {
        QPalette palette;
        palette = ui->editFind->palette();
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, Qt::black);
        ui->editFind->setPalette(palette);
      }
    }
  }
}

void SearchThread::run() {
  MainWindow::searchInFolders();
  emit isDone();
}

void MainWindow::dealDone() {
  ui->treeFind->clear();
  if (isBreakFind) {
    tw_SearchResults.removeAt(tw_SearchResults.count() - 1);
  }
  ui->treeFind->addTopLevelItems(tw_SearchResults);

  QString strMatches =
      "    " + tr("Matches") + " : " + QString::number(totalPos);
  ui->lblSearch->setText(tr("Files") + " : " + QString::number(files.count()) +
                         "    " + tr("Results") + " : " +
                         QString::number(tw_SearchResults.count()) +
                         strMatches);
  ui->progressBar->setMaximum(100);
  isFinishFind = true;
  isBreakFind = false;
  tmeShowFindProgress->stop();
}

void MainWindow::searchInFolders() {
  QDir dir(findPath);
  QStringList nameFilters;
  nameFilters << "*.dsl"
              << "*.asl"
              << "*.c"
              << "*.cpp"
              << "*.h"
              << "*.hpp"
              << "*.mm"
              << "*.bat"
              << "*.txt"
              << "*.py"
              << "*.md"
              << "*.xml"
              << "*.java"
              << "*.kt"
              << "*.pro"
              << "*.gradle"
              << "*.plist";

  QStringList fmt =
      QString(
          "dsl;asl;c;cpp;h;hpp;mm;bat;txt;py;md;xml;java;kt;pro;gradle;plist")
          .split(';');

  files.clear();
  if (!isIncludeSubDir) {
    QStringList filesTemp =
        dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);

    for (int j = 0; j < filesTemp.count(); j++) {
      if (isBreakFind) {
        break;
        return;
      }

      if (filesTemp.at(j).mid(0, 1) != ".")
        files.append(findPath + "/" + filesTemp.at(j));
    }

    for (int i = 0; i < files.count(); i++) {
      if (isBreakFind) {
        break;
        return;
      }

      searchMain(files.at(i));
    }
  } else {
    Methods::getAllFiles(findPath, files, fmt);
  }
}

void MainWindow::searchMain(QString file) {
  QFile openfile(file);
  if (!openfile.open(QFile::ReadOnly | QFile::Text)) {
    return;
  }
  QTextStream in(&openfile);
  QString text = in.readAll();
  textEditSerach->clear();
  textEditSerach->setText(text);
  curFindFile = file;

  highlighsearchtext(findStr, textEditSerach, file, true);

  clearSearchHighlight(textEditSerach);
}

void MainWindow::on_tabWidget_misc_tabBarClicked(int index) {
  if (index == 2) {
    ui->editFind->setFocus();
  }
}

void MainWindow::on_actionFindNext_triggered() { ui->btnNext->clicked(); }

void MainWindow::on_btnFolder_clicked() {
  QString dirpath = QFileDialog::getExistingDirectory(
      this, tr("Set Folder"), "./", QFileDialog::ShowDirsOnly);
  ui->editFolder->setText(dirpath);
}

void MainWindow::on_btnExpand_clicked() {
  if (ui->btnExpand->text() == "E") {
    ui->treeFind->expandAll();
    ui->btnExpand->setText("C");
  } else if (ui->btnExpand->text() == "C") {
    ui->treeFind->collapseAll();
    ui->btnExpand->setText("E");
  }
}

void MainWindow::on_actionFindPrevious_triggered() { ui->btnPrevious->click(); }

void MainWindow::on_actionFind_triggered() { on_btnSearch_clicked(); }

void MainWindow::on_chkSubDir_clicked(bool checked) {
  isIncludeSubDir = checked;
}

void MainWindow::on_cboxFindScope_currentIndexChanged(int index) {
  if (index == 2) {
    ui->frameInFolder->setHidden(false);
    ui->chkSubDir->setHidden(false);
  } else {
    ui->frameInFolder->setHidden(true);
    ui->chkSubDir->setHidden(true);
  }
}

void MainWindow::on_btnStopFind_clicked() {
  if (!isFinishFind) {
    isBreakFind = true;
    mySearchThread->quit();
    mySearchThread->wait();
    tmeShowFindProgress->stop();
  }
}

void MainWindow::on_ShowFindProgress() {
  ui->lblPos->setText(tr("Count") + " : " + QString::number(curFindPos));
  ui->editProgress->setText(curFindFile);

  QString strMatches =
      "    " + tr("Matches") + " : " + QString::number(totalPos);
  ui->lblSearch->setText(tr("Files") + " : " + QString::number(files.count()) +
                         "    " + tr("Results") + " : " +
                         QString::number(tw_SearchResults.count()) +
                         strMatches);

  int s = QTime::currentTime().second();
  if (s % 3 == 0) {
    return;
    if (tw_SearchResults.count() > 2) {
      ui->treeFind->clear();
      // tw_SearchResults.removeAt(tw_SearchResults.count() - 1);
      for (int i = 0; i < tw_SearchResults.count() - 2; i++) {
        QTreeWidgetItem* topItem = new QTreeWidgetItem();
        topItem->setText(0, tw_SearchResults.at(i)->text(0));
        topItem->setText(1, tw_SearchResults.at(i)->text(1));

        for (int j = 0; j < tw_SearchResults.at(i)->childCount(); j++) {
          QTreeWidgetItem* item = new QTreeWidgetItem(topItem);
          item->setText(0, tw_SearchResults.at(i)->child(j)->text(0));
          item->setText(1, tw_SearchResults.at(i)->child(j)->text(1));
        }

        ui->treeFind->addTopLevelItem(topItem);
      }
    }
  }
}

void MainWindow::on_cboxFindScope_currentTextChanged(const QString& arg1) {
  Q_UNUSED(arg1);
  ui->treeFind->clear();
}

void MainWindow::on_actionSet_Bookmark_triggered() {
  int row, col;
  textEdit->getCursorPosition(&row, &col);

  QString text = textEdit->text(row).trimmed();
  QString addStr = curFile + "|" + QString::number(row + 1) + "|" + text;
  bool re = false;
  for (int i = 0; i < listBookmarks.count(); i++) {
    if (listBookmarks.at(i) == addStr) {
      re = true;
      break;
    }
  }
  if (!re) listBookmarks.append(addStr);

  saveBookmarks();

  getBookmarks();
}

void MainWindow::getBookmarks() {
  if (curFile == "" || loading || listBookmarks.count() == 0) {
    ui->listBook->clear();
    ui->textEditNotes->clear();
    return;
  }

  QSettings Reg(strIniFile, QSettings::IniFormat);

  ui->listBook->clear();
  for (int i = 0; i < listBookmarks.count(); i++) {
    QString str = listBookmarks.at(i);
    QStringList list = str.split("|");
    if (list.count() == 3) {
      if (list.at(0) == curFile) {
        QString line = list.at(1);
        int num = line.toInt();
        if (textEdit->text(num - 1).trimmed() == list.at(2)) {
          ui->listBook->addItem(line);

          QString strNotes =
              Reg.value(curFile +
                        ui->listBook->item(ui->listBook->count() - 1)->text())
                  .toString();

          ui->listBook->item(ui->listBook->count() - 1)
              ->setToolTip(list.at(2) + "\n\n" + strNotes);

          setBookmarks(num - 1);
        }
      }
    }
  }

  int row, col;
  textEdit->getCursorPosition(&row, &col);
  ui->textEditNotes->clear();
  for (int i = 0; i < ui->listBook->count(); i++) {
    if (ui->listBook->item(i)->text() == QString::number(row + 1)) {
      ui->listBook->setCurrentRow(i);
      on_listBook_itemClicked(ui->listBook->currentItem());
      break;
    }
  }
}

void MainWindow::on_actionViewBookmarks_triggered() {
  getBookmarks();

  if (ui->frameBook->isHidden()) {
    ui->frameBook->setHidden(false);
    ui->actionViewBookmarks->setChecked(true);
  } else {
    ui->frameBook->setHidden(true);
    ui->actionViewBookmarks->setChecked(false);
  }
}

void MainWindow::on_listBook_itemClicked(QListWidgetItem* item) {
  Q_UNUSED(item);
  if (ui->listBook->count() == 0) return;

  textEdit->setFocus();
  int line = ui->listBook->currentItem()->text().toInt();
  textEdit->setCursorPosition(line - 1, 0);

  QSettings Reg(strIniFile, QSettings::IniFormat);
  ui->textEditNotes->setPlainText(
      Reg.value(curFile + ui->listBook->currentItem()->text()).toString());
}

void MainWindow::init_Bookmarks() {
  int line = 0;
  for (int i = 0; i < listBookmarks.count(); i++) {
    QString str = listBookmarks.at(i);
    QStringList list = str.split("|");
    if (list.at(0) == curFile) {
      listBookmarks.removeAt(i);
    }
  }

  // SCI_MARKERNEXT(line lineStart, int markerMask) → line
  for (int i = 1; i <= textEdit->lines(); i++) {
    line = textEdit->SendScintilla(QsciScintilla::SCI_MARKERNEXT, i, 1 << 4);
    if (line != -1) {
      i = line;

      QString addStr = curFile + "|" + QString::number(line + 1) + "|" +
                       textEdit->text(line).trimmed();

      listBookmarks.append(addStr);
    } else
      break;
  }

#if (QT_VERSION <= QT_VERSION_CHECK(5, 15, 0))
  listBookmarks = listBookmarks.toSet().toList();
#else
  listBookmarks = QSet<QString>(listBookmarks.begin(), listBookmarks.end())
                      .values();  //去重
#endif

  saveBookmarks();

  getBookmarks();
}

void MainWindow::saveBookmarks() {
  QSettings Reg(strIniFile, QSettings::IniFormat);
  Reg.setValue("bookcount", listBookmarks.count());
  for (int i = 0; i < listBookmarks.count(); i++) {
    Reg.setValue("book" + QString::number(i), listBookmarks.at(i));
  }
}

void MainWindow::on_btnDelBook_clicked() {
  if (ui->listBook->currentRow() < 0) return;
  int row = ui->listBook->currentRow();
  int line = ui->listBook->currentItem()->text().toInt();
  for (int i = 0; i < 20; i++) {
    textEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETE, line - 1,
                            4);  // 4号标签为书签
  }

  QString str = curFile + "|" + QString::number(line) + "|";
  for (int i = 0; i < listBookmarks.count(); i++) {
    QString str1 = listBookmarks.at(i);
    if (str1.contains(str)) {
      listBookmarks.removeAt(i);
      break;
    }
  }

  ui->listBook->takeItem(row);

  saveBookmarks();
}

void MainWindow::on_textEditNotes_textChanged() {
  if (ui->listBook->currentRow() < 0) return;

  QSettings Reg(strIniFile, QSettings::IniFormat);
  Reg.setValue(curFile + ui->listBook->currentItem()->text(),
               ui->textEditNotes->toPlainText());

  refreshItemTip(ui->listBook->currentRow());
}

void MainWindow::on_listBook_currentRowChanged(int currentRow) {
  if (currentRow < 0) return;
}

void MainWindow::on_listBook_itemSelectionChanged() {
  int currentRow = ui->listBook->currentRow();
  if (currentRow < 0) return;
}

void MainWindow::refreshItemTip(int currentRow) {
  QSettings Reg(strIniFile, QSettings::IniFormat);
  QString strNotes =
      Reg.value(curFile + ui->listBook->item(currentRow)->text()).toString();

  QString strTip;

  for (int j = 0; j < listBookmarks.count(); j++) {
    QString str0 = curFile + "|" + ui->listBook->item(currentRow)->text() + "|";
    QString str1 = listBookmarks.at(j);
    if (str1.contains(str0)) {
      str1.replace(str0, "");
      strTip = str1;
      break;
    }
  }

  ui->listBook->item(currentRow)->setToolTip(strTip + "\n\n" + strNotes);
}

void MainWindow::on_btnBookmark_clicked() {
  on_actionViewBookmarks_triggered();
  update();
  repaint();
}
