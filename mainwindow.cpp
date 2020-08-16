#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "myhighlighter.h"

#include <QPainter>
#include <QTextBlock>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //主菜单
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::btnOpen_clicked);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::btnSave_clicked);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::btnSaveAs_clicked);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

    connect(ui->actionGenerate, &QAction::triggered, this, &MainWindow::btnGenerate_clicked);
    connect(ui->actionCompiling, &QAction::triggered, this, &MainWindow::btnCompile_clicked);

    textEdit = new CodeEditor(this);
    connect(textEdit, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::textEdit_cursorPositionChanged);
    connect(textEdit, &QPlainTextEdit::textChanged, this, &MainWindow::textEdit_textChanged);

    QFontMetrics metrics(textEdit->font());
    textEdit->setTabStopWidth(2 * metrics.width(' '));




#ifdef Q_OS_WIN32
// win
   textEdit->setFont(QFont(tr("SauceCodePro Nerd Font"), 9));

#endif

#ifdef Q_OS_LINUX
// linux
   textEdit->setFont(QFont(tr("SauceCodePro Nerd Font"), 12));
#endif

#ifdef Q_OS_MAC
// mac
   textEdit->setFont(QFont(tr("SauceCodePro Nerd Font"), 13));

   ui->actionGenerate->setEnabled(false);

#endif

   QDesktopWidget* desktopWidget = QApplication::desktop();
   //QRect deskRect = desktopWidget->availableGeometry();  //可用区域
   QRect screenRect = desktopWidget->screenGeometry();  //屏幕区域
   int w = screenRect.width();
   ui->treeWidget->setMinimumWidth(300);
   ui->treeWidget->setMaximumWidth(w/3);
   ui->treeWidget->setColumnWidth(0 , w/3);

    textEdit->setLineWrapMode(textEdit->NoWrap);
    ui->editShowMsg->setLineWrapMode(ui->editShowMsg->NoWrap);
    ui->editShowMsg->setReadOnly(true);
    ui->editShowMsg->setMaximumHeight(200);
    ui->editShowMsg->setMinimumHeight(100);


    QSplitter *splitterMain = new QSplitter(Qt::Horizontal,this);
    splitterMain->addWidget(ui->treeWidget);
    QSplitter *splitterRight = new QSplitter(Qt::Vertical,splitterMain);
    splitterRight->setOpaqueResize(true);
    splitterRight->addWidget(textEdit);
    splitterRight->addWidget(ui->editShowMsg);
    //splitterMain->show();
    ui->gridLayout->addWidget(splitterMain);

    MyHighLighter *highlighter;
    highlighter = new MyHighLighter();
    highlighter->setDocument(textEdit->document());


    ui->treeWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << "Members" << "Lines"));
    //设置水平滚动条
    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setStretchLastSection(false);

    ui->treeWidget->setColumnHidden(1 , true);


}

MainWindow::~MainWindow()
{
    delete ui;


}
void MainWindow::about()
{

    QMessageBox::about(this , tr("About") ,
                       QString::fromLocal8Bit("<a style='color: blue;' href = https://github.com/ic005k/QtiASL>QtiASL Editor</a>"));
}
void MainWindow::loadFile(const QString &fileName)
{
   loading = true;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QTextStream in(&file);
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    //textEdit->setPlainText(in.readAll());
    textEdit->setPlainText(QString::fromUtf8(file.readAll()));
#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);

    ui->editShowMsg->clear();

    //刷新树表
    on_btnRefreshTree_clicked();


}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->document()->setModified(false);
    setWindowModified(false);

    QString shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    setWindowFilePath(shownName);

    setWindowTitle(shownName);
}


void MainWindow::btnOpen_clicked()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,"DSDT文件","","DSDT文件(*.aml *.dsl *.dat);;所有文件(*.*)");
        if (!fileName.isEmpty())
        {
            QFileInfo fInfo(fileName);
            if(fInfo.suffix() == "aml" || fInfo.suffix() == "dat")
            {
                QFileInfo appInfo(qApp->applicationDirPath());
                #ifdef Q_OS_WIN32
                // win
                    QProcess::execute(appInfo.filePath() + "/iasl.exe" , QStringList() << "-d" << fileName);
                #endif

                #ifdef Q_OS_LINUX
                // linux

                #endif

                #ifdef Q_OS_MAC
                // mac
                    QProcess::execute(appInfo.filePath() + "/iasl" , QStringList() << "-d" << fileName);
                #endif


                fileName = fInfo.path() + "/" + fInfo.baseName() + ".dsl";

            }

            loadFile(fileName);




        }


    }


}

bool MainWindow::maybeSave()
{
    if (!textEdit->document()->isModified())
        return true;
    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Application"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return btnSave_clicked();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}

bool MainWindow::btnSave_clicked()
{
    if (curFile.isEmpty()) {
        return btnSaveAs_clicked();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::btnSaveAs_clicked()
{
    QFileDialog dialog;
    QString fn = dialog.getSaveFileName(this,"DSDT文件","","DSDT文件(*.dsl);;所有文件(*.*)");
    if(fn.isEmpty())
        return false;

    return  saveFile(fn);

}

bool MainWindow::saveFile(const QString &fileName)
{
    QString errorMessage;

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    QSaveFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << textEdit->toPlainText();
        if (!file.commit()) {
            errorMessage = tr("Cannot write file %1:\n%2.")
                           .arg(QDir::toNativeSeparators(fileName), file.errorString());
        }
    } else {
        errorMessage = tr("Cannot open file %1 for writing:\n%2.")
                       .arg(QDir::toNativeSeparators(fileName), file.errorString());
    }
    QGuiApplication::restoreOverrideCursor();

    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Application"), errorMessage);
        return false;
    }

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);

    //刷新成员树
    on_btnRefreshTree_clicked();

    return true;
}

void MainWindow::btnGenerate_clicked()
{
    QFileInfo appInfo(qApp->applicationDirPath());
    qDebug() << appInfo.filePath();
    QString mydir = appInfo.filePath() + "/mydsdt";
    QDir dir;
    if(dir.mkpath(mydir))
    {

    }

    QProcess dump;
    QProcess iasl;


#ifdef Q_OS_WIN32
// win
    dump.execute(appInfo.filePath() + "/acpidump.exe" , QStringList() << "-b");//阻塞
    iasl.execute(appInfo.filePath() + "/iasl.exe" , QStringList() << "-d" << "dsdt.dat");
#endif

#ifdef Q_OS_LINUX
// linux

#endif

#ifdef Q_OS_MAC
// mac
    dump.execute(appInfo.filePath() + "/acpidump" , QStringList() << "-b");//阻塞
    iasl.execute(appInfo.filePath() + "/iasl" , QStringList() << "-d" << "dsdt.dat");

#endif



    //qDebug() << iasl.waitForStarted();

    loadFile(appInfo.filePath() + "/dsdt.dsl");


}

void MainWindow::btnCompile_clicked()
{
    QFileInfo appInfo(qApp->applicationDirPath());
    co = new QProcess;

    if(!curFile.isEmpty())
        btnSave_clicked();

#ifdef Q_OS_WIN32
// win
   co->start(appInfo.filePath() + "/iasl.exe" , QStringList() << "-f" << curFile);
#endif

#ifdef Q_OS_LINUX
// linux

#endif

#ifdef Q_OS_MAC
// mac
    co->start(appInfo.filePath() + "/iasl" , QStringList() << "-f" << curFile);

#endif




    connect(co , SIGNAL(finished(int)) , this , SLOT(readResult(int)));
    //connect(co , SIGNAL(readyReadStandardOutput()) , this , SLOT(readResult(int)));

    //QByteArray res = co.readAllStandardOutput(); //获取标准输出
    //qDebug() << "Out" << QString::fromLocal8Bit(res);

}

void MainWindow::setMark()
{
    //回到第一行
    QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(0);
    ui->editShowMsg->setTextCursor(QTextCursor(block));

    //将"error"高亮
    QString search_text = "Error";
        if (search_text.trimmed().isEmpty())
        {
            QMessageBox::information(this, tr("Empty search field"), tr("The search field is empty."));
        }
        else
        {
            QTextDocument *document = ui->editShowMsg->document();
            bool found = false;
            QTextCursor highlight_cursor(document);
            QTextCursor cursor(document);

            //开始
            cursor.beginEditBlock();
            QTextCharFormat color_format(highlight_cursor.charFormat());
            color_format.setForeground(Qt::red);
            color_format.setBackground(Qt::yellow);


            while (!highlight_cursor.isNull() && !highlight_cursor.atEnd())
            {
                //查找指定的文本，匹配整个单词
                highlight_cursor = document->find(search_text, highlight_cursor, QTextDocument::FindCaseSensitively);
                if (!highlight_cursor.isNull())
                {
                    if(!found)
                        found = true;

                    //const QTextCursor cursor = ui->editShowMsg->textCursor();
                    //int RowNum = cursor.blockNumber();

                    //QString str = document->findBlockByLineNumber(RowNum).text();
                    //QString sub = str.trimmed();
                    //qDebug() << str << RowNum;
                    //if(sub.mid(0 , 5) == "Error")
                    //{
                    highlight_cursor.mergeCharFormat(color_format);

                    //设置行高亮
                    /*QList<QTextEdit::ExtraSelection> extraSelection;
                    QTextEdit::ExtraSelection selection;
                    //QColor lineColor = QColor(Qt::gray).lighter(150);
                    QColor lineColor = QColor(Qt::red).lighter(150);
                    selection.format.setBackground(lineColor);
                    //selection.format.setForeground(Qt::black);
                    selection.format.setProperty(QTextFormat::FullWidthSelection , true);
                    selection.cursor = highlight_cursor; //ui->editShowMsg->textCursor();
                    selection.cursor.clearSelection();
                    extraSelection.append(selection);
                    ui->editShowMsg->setExtraSelections(extraSelection);*/
                    //}
                }
            }


           cursor.endEditBlock();

        }
}

void MainWindow::readResult(int exitCode)
{

    QTextCodec* gbkCodec = QTextCodec::codecForName("UTF-8");
    ui->editShowMsg->clear();
    QString result = gbkCodec->toUnicode(co->readAll());
    ui->editShowMsg->append(result);
    ui->editShowMsg->append(co->readAllStandardError());

    //回到第一行
    QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(0);
    ui->editShowMsg->setTextCursor(QTextCursor(block));

    if(exitCode == 0)
    {

        ui->btnPreviousError->setEnabled(false);
        ui->btnNextError->setEnabled(false);
        QMessageBox::information(this , "编译(Compiling)" , "编译成功(Compiled successfully)");

    }
    else
    {
        ui->btnPreviousError->setEnabled(true);
        ui->btnNextError->setEnabled(true);

        on_btnNextError_clicked();
    }

    //setMark();




}

void MainWindow::textEdit_cursorPositionChanged()
{
    const QTextCursor cursor = textEdit->textCursor();//QTextEdit 的当前光标
    int ColNum = cursor.columnNumber();//col
    int RowNum = cursor.blockNumber();//row

    //QTextCursor tc = textEdit->textCursor();
    //QTextLayout *pLayout = tc.block().layout();
    //int nCurpos = tc.position() - tc.block().position();
    //int nTextline = pLayout->lineForTextPosition(nCurpos).lineNumber() + tc.block().firstLineNumber();

    ui->statusbar->showMessage(QString::number(RowNum + 1) + "    " + QString::number(ColNum));

    //联动treeWidget

    if(!loading && ui->treeWidget->topLevelItemCount() > 0)
    {

        int cu;
        int cu1;

        for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
        {
            cu = ui->treeWidget->topLevelItem(i)->text(1).toUInt();
            if(i + 1 == ui->treeWidget->topLevelItemCount())
            {
                cu1 = cu;
            }
            else
            {
                cu1 = ui->treeWidget->topLevelItem(i + 1)->text(1).toUInt();
            }

            qDebug() << RowNum << cu << cu1;

            if(RowNum == cu)
            {

                ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(i));

                break;

            }
            else
            {
                if(RowNum > cu && RowNum < cu1)
                {
                    ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(i));
                    break;
                }

            }

         }

        //最后一个,单独处理
        int t = ui->treeWidget->topLevelItemCount();
        cu = ui->treeWidget->topLevelItem(t - 1)->text(1).toUInt();
        if(RowNum > cu)
        {

            ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(t - 1));

        }


    }



}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

//![extraAreaPaintEvent_0]

//![extraAreaPaintEvent_1]
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
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

void CodeEditor::highlightCurrentLine()
{
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


void MainWindow::on_btnReplace_clicked()
{
    textEdit->textCursor().insertText(ui->editReplace->text().trimmed());

}

void MainWindow::on_editShowMsg_textChanged()
{

}

void MainWindow::on_btnFindNext_clicked()
{
    //QPalette palette = textEdit->palette();
    //palette.setColor(QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
    //palette.setColor(QPalette::Highlight,QColor(0 , 0 , 255));

    //textEdit->setPalette(palette);
    //查找
    textEdit->find(ui->editFind->text().trimmed());
}

void MainWindow::on_btnFindPrevious_clicked()
{
    //QPalette palette = textEdit->palette();
    //palette.setColor(QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
    //palette.setColor(QPalette::Highlight,QColor(0 , 0 , 255));

    //textEdit->setPalette(palette);
    //查找
    textEdit->find(ui->editFind->text().trimmed() , QTextDocument::FindBackward);

}

void MainWindow::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{

    loading = true;

    if(column == 0)
    {
        int lines = item->text(1).toUInt();
        QTextBlock block = textEdit->document()->findBlockByNumber(lines);
        textEdit->setTextCursor(QTextCursor(block));

    }

    loading = false;

}

void MainWindow::on_btnRefreshTree_clicked()
{
    loading = true;

    ui->btnRefreshTree->setEnabled(false);

    //记住最开始的光标行位置
    const QTextCursor cursor = textEdit->textCursor();//QTextEdit 的当前光标
    int row = cursor.blockNumber();//row

    int count = textEdit->document()->lineCount();
    QString str;
    int RowNum;
    int ColNum;
    //bool scope = false;

    ui->treeWidget->clear();

    //回到第一行
    QTextBlock block = textEdit->document()->findBlockByNumber(0);
    textEdit->setTextCursor(QTextCursor(block));


    QTreeWidgetItem *twItem0;// , *twItem1 , *twItem2;

    //枚举"Scope"
    for(int j = 0; j < count; j++)
    {

        if(textEdit->find("Scope"))
        {

           const QTextCursor cursor = textEdit->textCursor();
           RowNum = cursor.blockNumber();

           ColNum = cursor.columnNumber() - 5 - 4;
           QString space;
           for(int y = 0; y < ColNum; y++)
           {
               space = space + " ";
           }

           qDebug() << ColNum;

           str = textEdit->document()->findBlockByLineNumber(RowNum).text();
           QString sub = str.trimmed();
           QString str_end;
           if(sub.mid(0 , 5) == "Scope")
           {
               for(int i = 0; i < sub.count(); i++)
               {
                   if(sub.mid(i , 1) == ")")
                   {
                       str_end = sub.mid(0 , i + 1);
                       break;

                   }
               }

               twItem0 = new QTreeWidgetItem(QStringList() << space + str_end << QString("%1").arg(RowNum, 6, 10, QChar('0')));    //QString::number(RowNum).arg( , 6));
               ui->treeWidget->addTopLevelItem(twItem0);
           }
         }
        else
            break;

    }

    //回到第一行
    block = textEdit->document()->findBlockByNumber(0);
    textEdit->setTextCursor(QTextCursor(block));

    for(int i = 0; i < count; i++)
    {


        if(textEdit->find("Device"))
        {

           const QTextCursor cursor = textEdit->textCursor();
           RowNum = cursor.blockNumber();

           ColNum = cursor.columnNumber() - 6 - 4;
           QString space;
           for(int y = 0; y < ColNum; y++)
           {
               space = space + " ";
           }

           str = textEdit->document()->findBlockByLineNumber(RowNum).text();
           QString sub = str.trimmed();
           QString str_end;
           if(sub.mid(0 , 6) == "Device")
           {
               for(int i = 0; i < sub.count(); i++)
               {
                   if(sub.mid(i , 1) == ")")
                   {
                       str_end = sub.mid(0 , i + 1);
                       break;

                   }
               }

               twItem0 = new QTreeWidgetItem(QStringList() << space + str_end << QString("%1").arg(RowNum, 6, 10, QChar('0')));//QString::number(RowNum));
               ui->treeWidget->addTopLevelItem(twItem0);



           }

        }
        else
            break;


    }

    //枚举所有的Method
    //回到第一行
    block = textEdit->document()->findBlockByNumber(0);
    textEdit->setTextCursor(QTextCursor(block));

    for(int j = 0; j < count; j++)
    {

        if(textEdit->find("Method"))
        {

           const QTextCursor cursor = textEdit->textCursor();
           RowNum = cursor.blockNumber();

           ColNum = cursor.columnNumber() - 6 - 4;
           QString space;
           for(int y = 0; y < ColNum; y++)
           {
               space = space + " ";
           }

           str = textEdit->document()->findBlockByLineNumber(RowNum).text();
           QString sub = str.trimmed();

           bool zs = false;
           for(int k = ColNum + 4; k > -1; k--)
           {
               if(str.mid(k - 2 , 2) == "//" || str.mid(k - 2 , 2) == "/*")
               {
                   zs = true;
                   //qDebug() << str.mid(k - 2 , 2);
                   break;
               }


           }

           QString str_end;
           if(sub.mid(0 , 6) == "Method" && zs == false)
           {

               for(int i = 0; i < sub.count(); i++)
               {
                   if(sub.mid(i , 1) == ")")
                   {
                       str_end = sub.mid(0 , i + 1);
                       break;

                   }
               }

               twItem0 = new QTreeWidgetItem(QStringList() << space + str_end << QString("%1").arg(RowNum, 6, 10, QChar('0')));//QString::number(RowNum));
               ui->treeWidget->addTopLevelItem(twItem0);
           }
         }
        else
            break;

    }


    //最后再回到当前行
    if(row > textEdit->document()->lineCount())
        row = textEdit->document()->lineCount() - 1;
    block = textEdit->document()->findBlockByNumber(row);
    textEdit->setTextCursor(QTextCursor(block));

    //ui->treeWidget->header()->sortIndicatorOrder();
    ui->treeWidget->sortItems(1 , Qt::AscendingOrder);


    ui->btnRefreshTree->setEnabled(true);

    loading = false;

}

void MainWindow::on_treeWidget_itemSelectionChanged()
{



}

void MainWindow::on_editShowMsg_cursorPositionChanged()
{
    QList<QTextEdit::ExtraSelection> extraSelection;
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::gray).lighter(150);
    //QColor lineColor = QColor(Qt::red).lighter(150);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection,true);
    selection.cursor = ui->editShowMsg->textCursor();
    //selection.cursor.clearSelection();
    extraSelection.append(selection);
    ui->editShowMsg->setExtraSelections(extraSelection);
}

void MainWindow::on_btnNextError_clicked()
{

    const QTextCursor cursor = ui->editShowMsg->textCursor();
    int RowNum = cursor.blockNumber();

    QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(RowNum);
    ui->editShowMsg->setTextCursor(QTextCursor(block));


    for(int i = RowNum + 1; i < ui->editShowMsg->document()->lineCount(); i++)
    {
        QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(i);
        ui->editShowMsg->setTextCursor(QTextCursor(block));


        QString str = ui->editShowMsg->document()->findBlockByLineNumber(i).text();
        QString sub = str.trimmed();


        if(sub.mid(0 , 5) == "Error")
        {
            QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(i);
            ui->editShowMsg->setTextCursor(QTextCursor(block));

            QList<QTextEdit::ExtraSelection> extraSelection;
            QTextEdit::ExtraSelection selection;
            //QColor lineColor = QColor(Qt::gray).lighter(150);
            QColor lineColor = QColor(Qt::red);
            selection.format.setForeground(Qt::white);
            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection,true);
            selection.cursor = ui->editShowMsg->textCursor();
            selection.cursor.clearSelection();
            extraSelection.append(selection);
            ui->editShowMsg->setExtraSelections(extraSelection);

            //定位到错误行
            getErrorLine(i);

            //qDebug() << str1;
            //qDebug() << str2;
            //qDebug() << str3;

            break;

        }



        if(i == ui->editShowMsg->document()->lineCount() - 1)
            on_btnPreviousError_clicked();
    }

}

void MainWindow::on_btnPreviousError_clicked()
{

    const QTextCursor cursor = ui->editShowMsg->textCursor();
    int RowNum = cursor.blockNumber();

    QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(RowNum);
    ui->editShowMsg->setTextCursor(QTextCursor(block));


    for(int i = RowNum - 1; i > -1; i--)
    {
        QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(i);
        ui->editShowMsg->setTextCursor(QTextCursor(block));


        QString str = ui->editShowMsg->document()->findBlockByLineNumber(i).text();
        QString sub = str.trimmed();

        if(sub.mid(0 , 5) == "Error")
        {
            QTextBlock block = ui->editShowMsg->document()->findBlockByNumber(i);
            ui->editShowMsg->setTextCursor(QTextCursor(block));

            QList<QTextEdit::ExtraSelection> extraSelection;
            QTextEdit::ExtraSelection selection;
            //QColor lineColor = QColor(Qt::gray).lighter(150);
            QColor lineColor = QColor(Qt::red);
            selection.format.setForeground(Qt::white);
            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection,true);
            selection.cursor = ui->editShowMsg->textCursor();
            selection.cursor.clearSelection();
            extraSelection.append(selection);
            ui->editShowMsg->setExtraSelections(extraSelection);

            //定位到错误行
            getErrorLine(i);

            break;

        }

        if(i == 0)
            on_btnNextError_clicked();
    }

}

void MainWindow::getErrorLine(int i)
{
    //定位到错误行
    QString str1 = ui->editShowMsg->document()->findBlockByLineNumber(i - 1).text().trimmed();
    QString str2 , str3;
    if(str1 != "")
    {
        for(int j = 3; j < str1.count(); j++)
        {

            if(str1.mid(j , 1) == ":")
            {
                str2 = str1.mid(0 , j);

                break;
            }
        }

        for(int k = str2.count(); k > 0; k--)
        {
            if(str2.mid(k - 1 , 1) == " ")
            {
                str3 = str2.mid(k , str2.count() - k);

                //定位到错误行
                QTextBlock block = textEdit->document()->findBlockByNumber(str3.toUInt() - 1);
                textEdit->setTextCursor(QTextCursor(block));

                break;
            }
        }
    }
}

void MainWindow::on_editShowMsg_selectionChanged()
{
    QString row = ui->editShowMsg->textCursor().selectedText();
    int row_num = row.toUInt();
    if(row_num > 0)
    {
        QTextBlock block = textEdit->document()->findBlockByNumber(row_num - 1);

        textEdit->setTextCursor(QTextCursor(block));


    }


}

void MainWindow::textEdit_textChanged()
{
    if(!loading)
    {
        //on_btnRefreshTree_clicked();
    }
}

void MainWindow::on_editFind_returnPressed()
{
    on_btnFindNext_clicked();
}
