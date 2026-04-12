#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Qsci/qsciapis.h>
#include <Qsci/qscilexercoffeescript.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexermarkdown.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qscilexerverilog.h>
#include <Qsci/qsciscintilla.h>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCompleter>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QFileSystemModel>
#include <QFontDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMetaType>
#include <QMimeData>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSaveFile>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>
#include <QWindow>

#include "autoupdatedialog.h"
#include "dlgdecompile.h"
#include "dlgpreferences.h"
#include "minidialog.h"
#include "recentfiles.h"
#include "ui_dlgdecompile.h"
#include "ui_dlgpreferences.h"

//网络相关头文件
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
// JSON相关头文件
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef Q_OS_WIN32
#include <Shlobj.h>
#include <stdio.h>
#include <windows.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
class LineNumberArea;
class QsciScintilla;
class thread_one;
class SearchThread;
class MiniEditor;
class MaxEditor;
class MyWidget;

void refreshTree();
void getMembers(QString str_member, QsciScintilla* textEdit);
void getMemberTree(QsciScintilla* textEdit);
QString findKey(QString str, QString stf_sub, int f_null);

int getBraceScope(int start, int count, QsciScintilla* textEdit);
bool chkMemberName(QString str, QString name);

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();
  Ui::MainWindow* ui;

  QString strIniFile;
  QString strAppName = "Xiasl";
  QStringList listBookmarks;
  static void highlighsearchtext(QString searchText, QsciScintilla* textEdit,
                                 QString file, bool addTreeItem);
  static void clearSearchHighlight(QsciScintilla* textEdit);
  static void searchInFolders();
  SearchThread* mySearchThread;
  void dealDone();
  void on_StartSearch(QsciScintilla* textEdit, QString file);
  QVBoxLayout* hboxLayout;
  QTimer* tmrWatchPos;
  QMenu* mnuRecentOpenFile;
  QStringList recentFileList;
  void setRecentFiles(QString fileName);
  void addFilesWatch();
  void removeFilesWatch();
  QString getCurrentFileName(int index);
  QString strModiFile;
  bool blAutoCheckUpdate = false;
  QStringList reLoadByModiList;
  dlgPreferences* dlgset;
  void set_Font();
  void set_wrap();
  QLabel* lblEncoding;

  int getDockWidth();

  int getMiniDockX();

  int getTabWidgetEditX();

  int getTabWidgetEditW();

  int getMainWindowHeight();

  QString getTabTitle();

  void setMark();
  void about();
  void getErrorLine(int i);

  void loadFile(const QString& fileName, int row, int col);

  QsciScintilla* textEdit;
  QsciScintilla* getCurrentEditor(int index);

  MiniEditor* miniEdit;

  void setCurrentFile(const QString& fileName);

  void update_ui_tree();

  void loadLocal();

  QProcess* co;

  QProcess* Decompile;

  QProcess* pk;

  RecentFiles* m_recentFiles;

  RecentFiles* m_ssdtFiles;

  int current_line = 0;

  thread_one* mythread;  //线程对象

  QSplitter* splitterMain;

  void getACPITables(bool ssdt);

  void msg(int value);
  void msgstr(QString str);

  void checkReloadFilesByModi();

  void ShowAutoUpdateDlg(bool Database);

  AutoUpdateDialog* dlgAutoUpdate;
  int deleteDirfile(QString dirName);
  QString getUrl(QVariantList list);
  bool mac = false;
  bool win = false;
  bool linuxOS = false;
  bool osx1012 = false;
  QString getProxy();
  QStringList listMd5;
  QString getMD5(QString targetFile);
  QString getMD5FromList(QString file);

  QString treeViewStyleLight =
      "QTreeView::branch:hover {background-color:rgba(127,255,0,50)}\
      QTreeView::branch:selected {background: rgba(180 ,209 ,255, 255);selection-background-color:rgba(180 ,209 ,255, 255);}\
      QTreeView::item:hover{background-color:rgba(127,255,0,50)}\
      QTreeView::item:selected{background-color:rgba(180 ,209 ,255, 255); color:rgba(5,5,5,255);}"

      "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:"
      "open:has-children:has-siblings {image: url(:/icon/sub.svg);}"

      "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:"
      "closed:has-children:has-siblings {image: url(:/icon/main.svg);}";

  QString treeViewStyleDark =
      "QTreeView::branch:hover {background-color:rgba(127,255,0,50)}\
       QTreeView::branch:selected {background: rgba(66 ,92 ,141, 255);selection-background-color:rgba(66 ,92 ,141, 255);}\
       QTreeView::item:hover{background-color:rgba(127,255,0,50)}\
       QTreeView::item:selected{background-color:rgba(66 ,92 ,141, 255); color:rgba(226,230,237,255);}"

      "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:"
      "open:has-children:has-siblings {image: url(:/icon/sub.svg);}"

      "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:"
      "closed:has-children:has-siblings {image: url(:/icon/main.svg);}";

  QString treeFindStyleLight =
      "QTreeView::branch:hover {background-color:rgba(127,255,0,50)}"

      "QTreeView::branch:selected {background: rgb(180 ,209 ,255);"
      "selection-background-color:rgb(180 ,209 ,255);}"

      "QTreeWidget::item:hover{background-color:rgba(127,255,0,50)}"

      "QTreeWidget::item:selected{background-color:rgb(180 ,209 ,255); "
      "color:rgb(5,5,5);} "

      "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:"
      "open:has-children:has-siblings {image: url(:/icon/sub.svg);}"

      "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:"
      "closed:has-children:has-siblings {image: url(:/icon/main.svg);}";

  QString treeFindStyleDark =
      "QTreeView::branch:hover {background-color:rgba(127,255,0,50)}"

      "QTreeView::branch:selected {background: rgb(66 ,92 ,141);"
      "selection-background-color:rgb(66 ,92 ,141);}"

      "QTreeWidget::item:hover{background-color:rgba(127,255,0,50)}"

      "QTreeWidget::item:selected{background-color:rgb(66 ,92 ,141); "
      "color:rgb(226,230,237);} "

      "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:"
      "open:has-children:has-siblings {image: url(:/icon/sub.svg);}"

      "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:"
      "closed:has-children:has-siblings {image: url(:/icon/main.svg);}";

  QString treeWidgetStyleLight =
      "QTreeView::branch:hover {background-color:rgba(127,255,0,50)}"

      "QTreeView::branch:selected {background: rgb(180 ,209 ,255);"
      "selection-background-color:rgb(180 ,209 ,255);}"

      "QTreeWidget::item:hover{background-color:rgba(127,255,0,50)}"

      "QTreeWidget::item:selected{background-color:rgb(180 ,209 ,255); "
      "color:rgb(5,5,5);} "

      "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:"
      "open:has-children:has-siblings {image: url(:/icon/sub.svg);}"

      "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:"
      "closed:has-children:has-siblings {image: url(:/icon/main.svg);}"

      "QTreeView::branch:has-siblings:!adjoins-item{border-image:url(:/icon/"
      "branch-line.png)0;}"

      "QTreeView::branch:has-siblings:adjoins-item{border-image:url(:/icon/"
      "branch-more.png)0;}"

      "QTreeView::branch:!has-children:!has-siblings:adjoins-item{border-image:"
      "url(:/icon//branch-end.png)0;}";

  QString treeWidgetStyleDark =
      "QTreeView::branch:hover {background-color:rgba(127,255,0,50)}"

      "QTreeView::branch:selected {background: rgb(66 ,92 ,141);"
      "selection-background-color:rgb(66 ,92 ,141);}"

      "QTreeWidget::item:hover{background-color:rgba(127,255,0,50)}"

      "QTreeWidget::item:selected{background-color:rgb(66 ,92 ,141); "
      "color:rgb(226,230,237);} "

      "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:"
      "open:has-children:has-siblings {image: url(:/icon/sub.svg);}"

      "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:"
      "closed:has-children:has-siblings {image: url(:/icon/main.svg);}"

      "QTreeView::branch:has-siblings:!adjoins-item{border-image:url(:/icon/"
      "branch-line.png)0;}"

      "QTreeView::branch:has-siblings:adjoins-item{border-image:url(:/icon/"
      "branch-more.png)0;}"

      "QTreeView::branch:!has-children:!has-siblings:adjoins-item{border-image:"
      "url(:/icon//branch-end.png)0;}";

  QString infoShowStyleLight =
      "QListWidget::item::selected:active\
          {\
              color:black;\
              border-width:0;\
             background: rgb(180 ,209 ,255);\
          }\
          QListWidget::item:selected\
          {\
              color:black;\
              border-width:0;\
             background: rgb(180 ,209 ,255);\
          }";

  QString infoShowStyleDark =
      "QListWidget::item::selected:active\
          {\
              color:black;\
              border-width:0;\
             background: rgb(66 ,92 ,141);\
          }\
          QListWidget::item:selected\
          {\
              color:rgb(226,230,237);\
              border-width:0;\
             background: rgb(66 ,92 ,141);\
          }";

  QString sbarStyleLight =
      "QStatusBar { background: rgb(236, 236, 236);}\
          QStatusBar::item {\
                   border: 0px solid blue;\
                   border-radius: 3px;}";
  QString sbarStyleDark =
      "QStatusBar { background: rgb(50, 50, 50);}\
          QStatusBar::item {\
                   border: 0px solid red;\
                   border-radius: 3px;}";

  QString tabStyleLight =
      "QTabWidget::pane {\
              border: 1px solid gray;\
              background: rgb(236, 236, 236);\
        }\
        QTabBar::scroller {width:0}\
        QTabBar::close-button:hover {\
        image: url(:/icon/c0.png);\
        subcontrol-origin: padding;\
        subcontrol-position: bottom right;\
        }\
        QTabBar::close-button {\
        image: url(:/icon/c.png);\
        subcontrol-origin: padding;\
        subcontrol-position: bottom right;\
        } \
        QTabWidget::tab-bar:top {\
          top: 1px;\
        }\
        \
        QTabWidget::tab-bar:bottom {\
          bottom: 1px;\
        }\
        \
        QTabWidget::tab-bar:left {\
          right: 1px;\
        }\
        \
        QTabWidget::tab-bar:right {\
          left: 1px;\
        }\
        \
        QTabBar::tab {\
          border: 1px solid gray;\
        }\
        \
        QTabBar::tab:selected {\
          background: rgb(236, 236, 236);\
        }\
        \
        QTabBar::tab:!selected {\
          background: lightgray; \
        }\
        \
        QTabBar::tab:!selected:hover {\
          background: silver;\
        }\
        \
        QTabBar::tab:top:!selected {\
          margin-top: 3px;\
        }\
        \
        QTabBar::tab:bottom:!selected {\
          margin-bottom: 3px;\
        }\
        \
        QTabBar::tab:top, QTabBar::tab:bottom {\
          min-width: 8ex;\
          margin-right: -1px;\
          padding: 8px 20px 8px 20px;\
        }\
        \
        QTabBar::tab:top:selected {\
          border-bottom-color: none;\
        }\
        \
        QTabBar::tab:bottom:selected {\
          border-top-color: none;\
        }\
        \
        QTabBar::tab:top:last, QTabBar::tab:bottom:last,\
        QTabBar::tab:top:only-one, QTabBar::tab:bottom:only-one {\
          margin-right: 0;\
        }\
        \
        QTabBar::tab:left:!selected {\
          margin-right: 3px;\
        }\
        \
        QTabBar::tab:right:!selected {\
          margin-left: 3px;\
        }\
        \
        QTabBar::tab:left, QTabBar::tab:right {\
          min-height: 8ex;\
          margin-bottom: -1px;\
          padding: 10px 5px 10px 5px;\
        }\
        \
        QTabBar::tab:left:selected {\
          border-left-color: none;\
        }\
        \
        QTabBar::tab:right:selected {\
          border-right-color: none;\
        }\
        \
        QTabBar::tab:left:last, QTabBar::tab:right:last,\
        QTabBar::tab:left:only-one, QTabBar::tab:right:only-one {\
          margin-bottom: 0;\
        }";

  QString tabStyleDark =
      "QTabWidget::pane {\
        border: 1px solid gray;\
        background: rgb(60, 60, 60);\
        }\
        QTabBar::scroller {width:0}\
        QTabBar::close-button:hover {\
        image: url(:/icon/c0.png);\
        subcontrol-origin: padding;\
        subcontrol-position: bottom right;\
        }\
        \
        QTabBar::close-button {\
        image: url(:/icon/c.png);\
        subcontrol-origin: padding;\
        subcontrol-position: bottom right;\
        } \
        \
        QTabWidget::tab-bar:top {\
            top: 1px;\
        }\
        \
        QTabWidget::tab-bar:bottom {\
            bottom: 1px;\
        }\
        \
        QTabWidget::tab-bar:left {\
            right: 1px;\
        }\
        \
        QTabWidget::tab-bar:right {\
            left: 1px;\
        }\
        \
        QTabBar::tab {\
            border: 1px solid gray;\
        }\
        \
        QTabBar::tab:selected {\
            background: rgb(60, 60, 60);\
        }\
        \
        QTabBar::tab:!selected {\
            background: rgb(26.26,26); \
        }\
        \
        QTabBar::tab:!selected:hover {\
            /*background:  silver;\
            color: black;*/\
            \
        }\
        \
        QTabBar::tab:top:!selected {\
            margin-top: 3px;\
        }\
        \
        QTabBar::tab:bottom:!selected {\
            margin-bottom: 3px;\
        }\
        \
        QTabBar::tab:top, QTabBar::tab:bottom {\
            min-width: 8ex;\
            margin-right: -1px;\
            padding: 8px 20px 8px 20px;\
        }\
        \
        QTabBar::tab:top:selected {\
            border-bottom-color: none;\
        }\
        \
        QTabBar::tab:bottom:selected {\
            border-top-color: none;\
        }\
        \
        QTabBar::tab:top:last, QTabBar::tab:bottom:last,\
        QTabBar::tab:top:only-one, QTabBar::tab:bottom:only-one {\
            margin-right: 0;\
        }\
        \
        QTabBar::tab:left:!selected {\
            margin-right: 3px;\
        }\
        \
        QTabBar::tab:right:!selected {\
            margin-left: 3px;\
        }\
        \
        QTabBar::tab:left, QTabBar::tab:right {\
            min-height: 8ex;\
            margin-bottom: -1px;\
            padding: 10px 5px 10px 5px;\
        }\
        \
        QTabBar::tab:left:selected {\
            border-left-color: none;\
        }\
        \
        QTabBar::tab:right:selected {\
            border-right-color: none;\
        }\
        \
        QTabBar::tab:left:last, QTabBar::tab:right:last,\
        QTabBar::tab:left:only-one, QTabBar::tab:right:only-one {\
            margin-bottom: 0;\
        }";

  void init_RecentOpenMenuItem();
  void init_MiniEdit();

  MiniEditor* getCurrentMiniEditor(int index);

  void init_ScrollBox();
  bool AddCboxFindItem = false;

  static void searchMain(QString file);

  void setBookmarks(int linenr);

  void getBookmarks();

  void init_Bookmarks();
  void saveBookmarks();

  void refreshItemTip(int currentRow);

  QsciLexer* init_Lexer(QString file);

  void init_MiniText();

  QFont get_Font();

  int get_Red();

 protected:
  void closeEvent(QCloseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* e) override;
  void dropEvent(QDropEvent* e) override;
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  void mouseMoveEvent(QMouseEvent*) override;
  void moveEvent(QMoveEvent*) override;
  bool event(QEvent* event) override;

  void resizeEvent(QResizeEvent* event) override;
 public slots:
  void Open();
  QString openFile(QString fileName);
  void dealover();  //处理线程返回的结束信号

  bool eventFilter(QObject*, QEvent*) override;

  void on_NewWindow();

  void on_tabWidget_textEdit_currentChanged(int index);
  void on_tabWidget_textEdit_tabBarClicked(int index);

  void miniEdit_cursorPositionChanged();
  virtual void changeEvent(QEvent* event) override;
  void setValue();
  void setValue2();

 private slots:

  void on_actionExpandAll();

  void on_actionCollapseAll();

  void on_actionOpenDir();

  void treeWidgetBack_itemClicked(QTreeWidgetItem* item, int column);

  void on_miniMap();

  void on_clearFindText();

  void ds_Decompile();

  void view_info();

  void view_mem_list();

  void closeTab(int index);

  void kextstat();

  void on_RefreshTree();

  void on_StartRefreshThread();

  void timer_linkage();

  void readResult(int exitCode);
  void readCppResult(int exitCode);
  void readCppRunResult(int exitCode);
  void readDecompileResult(int exitCode);
  void readHelpResult(int exitCode);

  void readKextstat();

  void recentOpen(QString filename);
  void ssdtOpen(QString filename);

  void newFile(QString file);

  bool Save();

  bool SaveAs();

  void btnGenerate_clicked();

  void btnCompile_clicked();

  void textEdit_cursorPositionChanged();

  void textEdit_linesChanged();

  void textEdit_textChanged();

  void on_btnReplace();

  void on_btnFindPrevious();

  void on_treeWidget_itemClicked(QTreeWidgetItem* item, int column);

  void on_editShowMsg_cursorPositionChanged();

  void on_PreviousError();
  void on_NextError();

  void on_editShowMsg_selectionChanged();

  void on_editFind_ReturnPressed();

  void on_MainWindow_destroyed();

  void on_editErrors_cursorPositionChanged();

  void on_editWarnings_cursorPositionChanged();

  void on_editRemarks_cursorPositionChanged();

  void on_editOptimizations_cursorPositionChanged();

  void on_btnReplaceFind();

  void on_btnCompile();

  void on_treeView_doubleClicked(const QModelIndex& index);

  void on_btnReturn_clicked();

  void on_treeView_expanded(const QModelIndex& index);

  void on_treeView_collapsed(const QModelIndex& index);

  void iaslUsage();

  void replyFinished(QNetworkReply* reply);
  void CheckUpdate();

  void ReplaceAll();

  void on_editFind_editTextChanged(const QString& arg1);

  void on_editFind_currentIndexChanged(const QString& arg1);

  void on_listWidget_itemSelectionChanged();

  void on_tabWidget_misc_currentChanged(int index);

  void on_editFind_currentTextChanged(const QString& arg1);

  void on_actionQuit_triggered();

  void on_actionClose_tab_triggered();

  void on_btnNo_clicked();

  void on_btnYes_clicked();

  void on_actionDownload_Upgrade_Packages_triggered();
  void on_actProxy1_triggered();

  void on_actProxy2_triggered();

  void on_actProxy3_triggered();

  void on_actProxy4_triggered();

  void on_actProxy5_triggered();

  void on_actionPreferences_triggered();

  void on_btnNext_clicked();

  void on_btnPrevious_clicked();

  void on_btnDone_clicked();

  void on_btnReplace_clicked();

  void on_btnReplaceFind_clicked();

  void on_btnReplaceAll_clicked();

  void on_btnFind_clicked();

  void on_btnCompile_clicked();

  void on_btnErrorP_clicked();

  void on_btnErrorN_clicked();

  void on_btnCaseSensitive_clicked();

  void on_btnSave_clicked();

  void on_btnNew_clicked();

  void on_btnShowRepace_clicked();

  void on_actionReporting_Issues_triggered();

  void on_actionUser_Guide_triggered();

  void on_actionLatest_Release_triggered();

  void on_btnTabList_clicked();

  void on_actionAutomatic_Line_Feeds_triggered();

  void on_btnMiniMap_clicked();

  void on_actionNew_triggered();

  void on_treeFind_itemClicked(QTreeWidgetItem* item, int column);

  void on_btnSearch_clicked();

  void on_tabWidget_misc_tabBarClicked(int index);

  void on_actionFindNext_triggered();

  void on_btnFolder_clicked();

  void on_btnExpand_clicked();

  void on_actionFindPrevious_triggered();

  void on_actionFind_triggered();

  void on_chkSubDir_clicked(bool checked);

  void on_cboxFindScope_currentIndexChanged(int index);

  void on_btnStopFind_clicked();

  void on_ShowFindProgress();

  void on_cboxFindScope_currentTextChanged(const QString& arg1);

  void on_actionSet_Bookmark_triggered();

  void on_actionViewBookmarks_triggered();

  void on_listBook_itemClicked(QListWidgetItem* item);

  void on_btnDelBook_clicked();

  void on_textEditNotes_textChanged();

  void on_listBook_currentRowChanged(int currentRow);

  void on_listBook_itemSelectionChanged();

  void on_btnBookmark_clicked();

 private:
  QMenu* menuTabList;
  bool isDrag;
  QPoint m_position;
  int lblNumber = 2;
  int editNumber = 1;

  dlgDecompile* dlg;
  int index_treeFindChild = 0;

  QStringList findTextList;
  void init_findTextList();

  void forEach(QString str, QString strReplace);

  QNetworkAccessManager* manager;
  int parse_UpdateJSON(QString str);

  void loadTabFiles();

  QString shownName;

  void set_return_text(QString text);

  QString fsm_Filepath;

  QModelIndex fsm_Index;

  QString getLayerName(QTreeWidgetItem* hItem);

  int treeCount(QTreeWidget* tree, QTreeWidgetItem* parent);

  int treeCount(QTreeWidget* tree);

  bool maybeSave(QString info);

  bool find_up;

  bool find_down;

  QString ver;

  QTimer* timer;
  QTimer* tmeShowFindProgress;

  QTextEdit* textEditTemp;

  QLabel* lblMsg;

  QLabel* lblLayer;

  QLabel* lblCurrentFile = new QLabel;

  QLineEdit* editLayer;

  QElapsedTimer qTime;

  int row = 0;

  int row_current = 0;

  bool linkage = false;

  int preRow = 0;

  bool saveFile(const QString& fileName);

  void mem_linkage(QTreeWidget* tw, int RowNum);

  void init_menu();

  void init_Tool_UI();

  void init_recentFiles();

  void init_info_edit();

  void init_Edit();

  void init_treeWidget();

  void init_statusBar();
  QLabel* locationLabel;

  void init_filesystem();
  QFileSystemModel* model;

  void update_ui_tw();

  void separ_info(QString str_key, QTextEdit* editInfo);

  void set_cursor_line_color(QTextEdit* edit);

  void gotoLine(QTextEdit* edit);

  void fileAndprog_Linux();

  void regACPI_win();

  void set_MyStyle(QsciLexer* textLexer, QsciScintilla* textEdit);

  void update_member(bool show, QString str_void,
                     QList<QTreeWidgetItem*> tw_list);

  void set_mark(int linenr);

  bool DeleteDirectory(const QString& path);
  bool enterEdit(QPoint pp, QsciScintilla* btn);
  void on_btnNextError();
  void on_btnPreviousError();
  void goCppPreviousError();
  void goCppNextError();
  void getCppErrorLine(int i);
  void setErrorMarkers(int linenr);
  bool InfoWinShow = false;
  void setEditFindCompleter();

  void loadFindString();
  const QString iniFile = QDir::homePath() + "/.config/QtiASL/QtiASL.ini";

#ifndef QT_NO_CONTEXTMENU
  void contextMenuEvent(QContextMenuEvent* event) override;
#endif  // QT_NO_CONTEXTMENU
  void writeINIProxy();
  void readINIProxy();
  void updateMd5(QString file);
  void init_UIStyle();
  void setTextModifyMark();
  void init_TabList();
  void init_fsmSyncOpenedFile(QString OpenedFile);
  void init_listForRecentFile(QString fileName);
  void setEditFindMarker();
  void init_UI_Layout();
  void init_Widget();
};

class MiniEditor : public QsciScintilla {
  Q_OBJECT

 public:
  MiniEditor(QWidget* parent = nullptr);
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void showZoomWin(int x, int y);
  int miniLineNum = 0;
  QString currentLineText;
  int p0 = 0;

 public slots:
  bool eventFilter(QObject*, QEvent*) override;

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  // void mouseDoubleClickEvent(QMouseEvent* event) override;
  // void mouseMoveEvent(QMouseEvent* event) override;
  // void paintEvent(QPaintEvent*);

 private slots:
  void miniEdit_cursorPositionChanged();
  void miniEdit_verticalScrollBarChanged();

 private:
  int curY = 0;
};

class MaxEditor : public QsciScintilla {
  Q_OBJECT

 public:
  MaxEditor(QWidget* parent = nullptr);

 protected:
  void mouseMoveEvent(QMouseEvent* event) override;
 private slots:

 private:
};

class CodeEditor : public QPlainTextEdit {
  Q_OBJECT

 public:
  CodeEditor(QWidget* parent = nullptr);

  void lineNumberAreaPaintEvent(QPaintEvent* event);
  int lineNumberAreaWidth();

 protected:
  void resizeEvent(QResizeEvent* event) override;

 private slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void highlightCurrentLine();
  void updateLineNumberArea(const QRect& rect, int dy);

 private:
  QWidget* lineNumberArea;
};

class LineNumberArea : public QWidget {
 public:
  LineNumberArea(CodeEditor* editor) : QWidget(editor), codeEditor(editor) {}

  QSize sizeHint() const override {
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    codeEditor->lineNumberAreaPaintEvent(event);
  }

 private:
  CodeEditor* codeEditor;
};

class thread_one : public QThread {
  Q_OBJECT
 public:
  explicit thread_one(QObject* parent = nullptr);

 protected:
  void run();
 signals:
  void over();
 public slots:
};

class SearchThread : public QThread {
  Q_OBJECT
 public:
  explicit SearchThread(QObject* parent = nullptr);

 protected:
  void run();
 signals:
  void isDone();  //处理完成信号

 signals:

 public slots:
};

class QscilexerCppAttach : public QsciLexerCPP {
  Q_OBJECT
 public:
  const char* keywords(int set) const;
};

class MyWidget : public QGridLayout {
 public:
  QSize sizeHint() const {
    return QSize(600, 180); /* 在这里定义dock的初始大小 */
  }
};

#endif  // MAINWINDOW_H
