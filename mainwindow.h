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

QT_BEGIN_NAMESPACE

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class LineNumberArea;

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
    QPlainTextEdit *textEdit;

    int current_line = 0;

    bool loading = true;


private slots:
    void readResult(int exitCode);

    void btnOpen_clicked();

    bool btnSave_clicked();

    bool btnSaveAs_clicked();

    void btnGenerate_clicked();

    void btnCompile_clicked();

    void textEdit_cursorPositionChanged();

    void textEdit_textChanged();

    void on_btnReplace_clicked();

    void on_editShowMsg_textChanged();

    void on_btnFindNext_clicked();

    void on_btnFindPrevious_clicked();

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_btnRefreshTree_clicked();

    void on_treeWidget_itemSelectionChanged();

    void on_editShowMsg_cursorPositionChanged();

    void on_btnNextError_clicked();

    void on_btnPreviousError_clicked();

    void on_editShowMsg_selectionChanged();

    void on_editFind_returnPressed();

private:
    Ui::MainWindow *ui;

    void setCurrentFile(const QString &fileName);



    bool maybeSave();

    bool saveFile(const QString &fileName);

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


#endif // MAINWINDOW_H
