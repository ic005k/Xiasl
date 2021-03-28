#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dlgdecompile.h"
#include "minidialog.h"
#include "recentfiles.h"

#include <Qsci/qsciapis.h>
#include <Qsci/qscilexercoffeescript.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexerverilog.h>
#include <Qsci/qsciscintilla.h>

#include <QApplication>
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
#include <QTranslator>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

//网络相关头文件
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
//JSON相关头文件
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

    void addFilesWatch();
    void removeFilesWatch();
    QString getCurrentFileName(int index);

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
    //MaxEditor* textEdit;
    //MaxEditor* getCurrentEditor(int index);

    //QsciScintilla* miniEdit;
    MiniEditor* miniEdit;

    void setCurrentFile(const QString& fileName);

    void update_ui_tree();

    void loadLocal();

    QString curFile;

    QProcess* co;

    QProcess* Decompile;

    QProcess* pk;

    RecentFiles* m_recentFiles;

    QFont font;

    RecentFiles* m_ssdtFiles;

    int current_line = 0;

    thread_one* mythread; //线程对象

    QSplitter* splitterMain;

    void getACPITables(bool ssdt);

    void msg(int value);
    void msgstr(QString str);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

public slots:
    void Open();
    QString openFile(QString fileName);
    void dealover(); //处理线程返回的结束信号

    bool eventFilter(QObject*, QEvent*) override;

    void on_NewWindow();

    void on_tabWidget_textEdit_currentChanged(int index);
    void on_tabWidget_textEdit_tabBarClicked(int index);

    void miniEdit_cursorPositionChanged();

    void setValue();
    void setValue2();

private slots:
    void on_actionExpandAll();

    void on_actionCollapseAll();

    void on_actionOpenDir();

    void treeWidgetBack_itemClicked(QTreeWidgetItem* item, int column);

    void on_miniMap();

    void on_clearFindText();

    void userGuide();

    void ds_Decompile();

    void view_info();

    void view_mem_list();

    void closeTab(int index);

    void kextstat();

    void set_font();

    void set_wrap();

    void on_btnRefreshTree();

    void refresh_tree(QsciScintilla* textEdit);

    void timer_linkage();

    void readResult(int exitCode);
    void readCppResult(int exitCode);
    void readCppRunResult(int exitCode);
    void readDecompileResult(int exitCode);
    void readHelpResult(int exitCode);

    void readKextstat();

    void recentOpen(QString filename);
    void ssdtOpen(QString filename);

    void newFile();

    bool Save();

    bool SaveAs();

    void btnGenerate_clicked();

    void btnCompile_clicked();

    void textEdit_cursorPositionChanged();

    void textEdit_linesChanged();

    void textEdit_textChanged();

    void on_btnReplace();

    void on_btnFindNext();

    void on_btnFindPrevious();

    void on_treeWidget_itemClicked(QTreeWidgetItem* item, int column);

    void on_editShowMsg_cursorPositionChanged();

    void on_PreviousError();
    void on_NextError();

    void on_editShowMsg_selectionChanged();

    void editFind_returnPressed();

    void on_MainWindow_destroyed();

    void on_editErrors_cursorPositionChanged();

    void on_editWarnings_cursorPositionChanged();

    void on_editRemarks_cursorPositionChanged();

    void on_editOptimizations_cursorPositionChanged();

    void on_btnReplaceFind();

    void on_chkCaseSensitive_clicked();

    void on_chkCaseSensitive_clicked(bool checked);

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

private:
    Ui::MainWindow* ui;

    int lblNumber = 2;
    int editNumber = 1;

    dlgDecompile* dlg;

    void highlighsearchtext(QString searchText);
    void clearSearchHighlight(QsciScintilla* textEdit);
    QList<int> m_searchTextPosList;
    QString search_string;
    QLabel* lblCount;

    QStringList findTextList;
    void init_findTextList();

    void forEach(QString str, QString strReplace);

    QNetworkAccessManager* manager;
    int parse_UpdateJSON(QString str);
    bool mac = false;
    bool win = false;
    bool linuxOS = false;
    QString CurVerison = "V1.0";

    void loadTabFiles();

    bool One = false;

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

    bool CaseSensitive = false;

    QString ver;

    QTimer* timer;

    QTextEdit* textEditTemp;

    QLabel* lblMsg;

    QLabel* lblLayer;

    QLabel* lblEncoding;

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

    void init_toolbar();

    void init_recentFiles();

    void init_info_edit();

    void init_edit(QsciScintilla* textEdit);

    void init_miniEdit();

    void init_treeWidget();

    void init_statusBar();
    QLabel* locationLabel;

    void init_filesystem();
    QFileSystemModel* model;

    void update_ui_tw();

    void set_currsor_position(QsciScintilla* textEdit);

    void separ_info(QString str_key, QTextEdit* editInfo);

    void set_cursor_line_color(QTextEdit* edit);

    void gotoLine(QTextEdit* edit);

    void fileAndprog_Linux();

    void regACPI_win();

    void setLexer(QsciLexer* textLexer, QsciScintilla* textEdit);

    void update_member(bool show, QString str_void, QList<QTreeWidgetItem*> tw_list);

    void set_mark(int linenr);
    void syncMiniEdit();
    int deleteDirfile(QString dirName);
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

    void setVScrollBarStyle(int red);

    void loadFindString();
    const QString iniFile = QDir::homePath() + "/.config/QtiASL/QtiASL.ini";

#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent* event) override;
#endif // QT_NO_CONTEXTMENU
};

class MiniEditor : public QsciScintilla {
    Q_OBJECT

public:
    MiniEditor(QWidget* parent = nullptr);
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void showZoomWin(int x, int y);

protected:
    //void mousePressEvent(QMouseEvent* event) override;
    //void mouseDoubleClickEvent(QMouseEvent* event) override;
    //void mouseMoveEvent(QMouseEvent* event) override;
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
    LineNumberArea(CodeEditor* editor)
        : QWidget(editor)
        , codeEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
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

class QscilexerCppAttach : public QsciLexerCPP {
    Q_OBJECT
public:
    const char* keywords(int set) const;
};

class MyWidget : public QGridLayout {
public:
    QSize sizeHint() const
    {
        return QSize(600, 180); /* 在这里定义dock的初始大小 */
    }
};

#endif // MAINWINDOW_H
