#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QMessageBox>
#include <QDir>
#include <QTextStream>
#include <QFileDialog>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QTextCodec>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCodec>
#include <QTextBlock>
#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QDesktopWidget>
#include <QSplitter>
#include <QScreen>
#include <QDateTime>
#include <QTimer>
#include <QThread>
//#include "mythread.h"

class QsciScintilla;
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexerbatch.h>
#include <Qsci/qsciapis.h>
class QscilexerCppAttach : public QsciLexerCPP
{
    Q_OBJECT
public:
    const char *keywords(int set) const;
};


QT_BEGIN_NAMESPACE

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class LineNumberArea;
class thread_one;

void refreshTree(QTreeWidget *treeWidgetBack);
void getMembers(QString str_member, QsciScintilla *textEdit, QTreeWidget *treeWidget);
QString findKey(QString str, QString stf_sub, int f_null);


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadFile(const QString &fileName);
    void setMark();
    void about();
    void getErrorLine(int i);



    QString curFile;
    QProcess *co;
    QFont font;
    QsciScintilla *textEdit;

    int current_line = 0;

    thread_one *mythread; //线程对象
    QSplitter *splitterMain;




public slots:


private slots:
    void treeWidgetBack_itemClicked(QTreeWidgetItem *item, int column);

    void dealover();//处理新线程返回的结束信号

    void on_btnRefreshTree_clicked();

    void timer_linkage();

    void readResult(int exitCode);

    void btnOpen_clicked();

    bool btnSave_clicked();

    bool btnSaveAs_clicked();

    void btnGenerate_clicked();

    void btnCompile_clicked();

    void textEdit_cursorPositionChanged();

    void textEdit_linesChanged();

    void textEdit_textChanged();

    void on_btnReplace_clicked();

    void on_editShowMsg_textChanged();

    void on_btnFindNext_clicked();

    void on_btnFindPrevious_clicked();

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_editShowMsg_cursorPositionChanged();

    void on_btnNextError_clicked();

    void on_btnPreviousError_clicked();

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

private:
    Ui::MainWindow *ui;

    void setCurrentFile(const QString &fileName);

    bool maybeSave();

    QString ver;

    QTimer *timer;

    QTextEdit *textEditTemp;

    int row = 0;
    int row_current = 0;
    bool linkage = false;

    bool saveFile(const QString &fileName);

    void mem_linkage(QTreeWidget * tw);

    void init_info_edit();

    void init_edit();

    void init_treeWidget(QTreeWidget *treeWidgetBack, int w);

    void update_ui_tw();

    void separ_info(QString str_key, QTextEdit *editInfo);

    void set_cursor_line_color(QTextEdit * edit);//光标所在行的背景色

    void gotoLine(QTextEdit *edit);


};

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();


protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

class thread_one : public QThread
{
    Q_OBJECT
public:
    explicit thread_one(QObject *parent = nullptr);
protected:
    void run();
signals:
    void over();
public slots:
};


#endif // MAINWINDOW_H
