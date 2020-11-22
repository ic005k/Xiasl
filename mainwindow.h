#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dlgdecompile.h"
#include "recentfiles.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDebug>
#include <QDesktopWidget>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QFileSystemModel>
#include <QFontDialog>
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
#include <QtDebug>

#ifdef Q_OS_WIN32
#include <Shlobj.h>
#include <stdio.h>
#include <windows.h>
#endif

#include <Qsci/qsciapis.h>
#include <Qsci/qscilexercoffeescript.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexerverilog.h>
#include <Qsci/qsciscintilla.h>

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

    void setMark();
    void about();
    void getErrorLine(int i);

    void loadFile(const QString& fileName, int row, int col);

    QsciScintilla* textEdit;

    void setCurrentFile(const QString& fileName);

    void update_ui_tree();

    void loadLocal();

    QString curFile;

    QProcess* co;

    QProcess* Decompile;

    QProcess* pk;

    QFont font;

    RecentFiles* m_recentFiles;

    int current_line = 0;

    thread_one* mythread; //线程对象

    QSplitter* splitterMain;

protected:
    void closeEvent(QCloseEvent* event);
    void dragEnterEvent(QDragEnterEvent* e);
    void dropEvent(QDropEvent* e);
    void paintEvent(QPaintEvent* event);

public slots:
    void Open();
    QString openFile(QString fileName);
    void dealover(); //处理线程返回的结束信号

    bool eventFilter(QObject*, QEvent*);

private slots:
    void treeWidgetBack_itemClicked(QTreeWidgetItem* item, int column);

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
    void readDecompileResult(int exitCode);

    void readKextstat();

    void recentOpen(QString filename);

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

    void on_btnNextError();

    void on_btnPreviousError();

    void on_editShowMsg_selectionChanged();

    void on_editFind_returnPressed();

    void on_MainWindow_destroyed();

    void on_chkScope_clicked();

    void on_chkDevice_clicked();

    void on_chkMethod_clicked();

    void on_chkName_clicked();

    void on_editErrors_cursorPositionChanged();

    void on_editWarnings_cursorPositionChanged();

    void on_editRemarks_cursorPositionChanged();

    void on_editOptimizations_cursorPositionChanged();

    void on_btnReplaceFind();

    void on_chkCaseSensitive_clicked();

    void on_chkCaseSensitive_clicked(bool checked);

    void on_editFind_textChanged(const QString& arg1);

    void on_btnCompile();

    void on_treeView_doubleClicked(const QModelIndex& index);

    void on_btnReturn_clicked();

    void on_treeView_expanded(const QModelIndex& index);

    void on_treeView_collapsed(const QModelIndex& index);

    void on_tabWidget_textEdit_tabBarClicked(int index);

    void on_tabWidget_textEdit_currentChanged(int index);

private:
    Ui::MainWindow* ui;

    dlgDecompile* dlg;

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

    int red;

    QString ver;

    QTimer* timer;

    QTextEdit* textEditTemp;

    QLabel* lblMsg;

    QLabel* lblLayer;

    QLineEdit* editLayer;

    QElapsedTimer qTime;

    int row = 0;

    int row_current = 0;

    bool linkage = false;

    int preRow = 0;

    bool saveFile(const QString& fileName);

    void mem_linkage(QTreeWidget* tw, int RowNum);

    void init_menu();

    void init_recentFiles();

    void init_info_edit();

    void init_edit(QsciScintilla* textEdit);

    void init_treeWidget();

    void init_statusBar();

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

#endif // MAINWINDOW_H
