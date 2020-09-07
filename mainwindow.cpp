#include "mainwindow.h"
#include "ui_mainwindow.h"


bool loading = true;
bool thread_end = true;
bool break_run = false;
bool show_s = true;
bool show_d = true;
bool show_m = true;
bool show_n = false;
int s_count = 0;
int m_count = 0;
int d_count = 0;
int n_count = 0;
QsciScintilla *textEditBack;
QList<QTreeWidgetItem *> twitems;
QList<QTreeWidgetItem *> tw_scope;
QList<QTreeWidgetItem *> tw_device;
QList<QTreeWidgetItem *> tw_method;
QList<QTreeWidgetItem *> tw_name;
QString fileName;
QVector<QString> filelist;
QWidgetList wdlist;
QscilexerCppAttach *textLexer;

thread_one::thread_one(QObject *parent) : QThread(parent)
{
}

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

    ver = "QtiASL V1.0.7a    ";
    setWindowTitle(ver);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    mythread = new thread_one();
    connect(mythread,&thread_one::over,this,&MainWindow::dealover);

    init_menu();

#ifdef Q_OS_WIN32
// win
    //QFont font("SauceCodePro Nerd Font", 9, QFont::Normal);
    font.setFamily("SauceCodePro Nerd Font");
    font.setPointSize(9);

    regACPI_win();

#endif

#ifdef Q_OS_LINUX
// linux
    font.setFamily("SauceCodePro Nerd Font");
    font.setPointSize(13);
#endif

#ifdef Q_OS_MAC
// mac
    font.setFamily("SauceCodePro Nerd Font");
    font.setPointSize(13);

    ui->actionGenerate->setEnabled(false);

#endif

    int w = screen()->size().width();
    init_treeWidget(ui->treeWidget, w);

    init_edit();

    init_info_edit();

    splitterMain = new QSplitter(Qt::Horizontal,this);
    splitterMain->addWidget(ui->treeWidget);
    QSplitter *splitterRight = new QSplitter(Qt::Vertical,splitterMain);
    splitterRight->setOpaqueResize(true);
    splitterRight->addWidget(textEdit);
    splitterRight->addWidget(ui->tabWidget);
    ui->gridLayout->addWidget(splitterMain);
    ui->tabWidget->setHidden(true);
    ui->treeWidget->setHidden(true);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_linkage()));

    //最近打开的文件
    //Mac:"/Users/../Library/Preferences/com.github-com-ic005k-qtiasl.V1.plist"
    //Win:"\\HKEY_CURRENT_USER\\Software\\QtiASL\\V1"
    QCoreApplication::setOrganizationName("QtiASL");
    QCoreApplication::setOrganizationDomain("github.com/ic005k/QtiASL");
    QCoreApplication::setApplicationName("V1");
    m_recentFiles = new RecentFiles(this);
    m_recentFiles->attachToMenuAfterItem(ui->menu_File, "另存-SaveAS...", SLOT(recentOpen(QString)));//在分隔符菜单项下插入
    m_recentFiles->setNumOfRecentFiles(15);//最多显示最近的15个文件

    lblMsg = new QLabel(this);
    ui->statusbar->addPermanentWidget(lblMsg);

    textEdit->setFocus();

}

MainWindow::~MainWindow()
{
    delete ui;

    mythread->quit();
    mythread->wait();

}
void MainWindow::about()
{
    QFileInfo appInfo(qApp->applicationFilePath());
    QString last = tr("Last modified: ") + appInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");

    QMessageBox::about(this , tr("About") ,
                       QString::fromLocal8Bit("<a style='color: blue;' href = https://github.com/ic005k/QtiASL>QtiASL Editor</a><br><br>") + last);
}

QString MainWindow::openFile(QString fileName)
{

    QSettings settings;
    QFileInfo fInfo(fileName);
    settings.setValue("currentDirectory", fInfo.absolutePath());
    //qDebug() << settings.fileName(); //最近打开的文件所保存的位置
    m_recentFiles->setMostRecentFile(fileName);

    if(fInfo.suffix() == "aml" || fInfo.suffix() == "dat")
    {
        QFileInfo appInfo(qApp->applicationDirPath());
        #ifdef Q_OS_WIN32
        // win
            QProcess::execute(appInfo.filePath() + "/iasl.exe" , QStringList() << "-d" << fileName);
        #endif

        #ifdef Q_OS_LINUX
        // linux
            QProcess::execute(appInfo.filePath() + "/iasl" , QStringList() << "-d" << fileName);

        #endif

        #ifdef Q_OS_MAC
        // mac
            QProcess::execute(appInfo.filePath() + "/iasl" , QStringList() << "-d" << fileName);
        #endif


        fileName = fInfo.path() + "/" + fInfo.baseName() + ".dsl";

    }

    QFileInfo fi(fileName);
    if(fi.suffix().toLower() == "dsl")
    {
        ui->actionWrapWord->setChecked(false);//取消自动换行，影响dsl文件开启速度
        textEdit->setWrapMode(QsciScintilla::WrapNone);
        ui->treeWidget->setHidden(false);
    }
    else
        ui->treeWidget->setHidden(true);
    ui->tabWidget->setHidden(true);


    return  fileName;
}

void MainWindow::loadFile(const QString &fileName)
{
   loading = true;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QTextStream in(&file);
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
#endif

    QString text;
    //关于是否采用GBK编码的方式，再考虑，目前基本不影响
    /*QTextCodec* gCodec = QTextCodec::codecForName("GBK");
    if(gCodec)
    {
        text = gCodec->toUnicode(file.readAll());
    }
    else
        text = QString::fromUtf8(file.readAll());*/

    text = QString::fromUtf8(file.readAll());
    textEdit->setText(text);

#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);

    ui->editShowMsg->clear();

    ui->treeWidget->clear();
    ui->treeWidget->repaint();

    on_btnRefreshTree_clicked();

    ui->treeWidget->repaint();

    loading = false;

}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->setModified(false);
    setWindowModified(false);

    QString shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.dsl";


    setWindowFilePath(shownName);

    setWindowTitle(ver + shownName);
}


void MainWindow::btnOpen_clicked()
{
    if (maybeSave())
    {
        fileName = QFileDialog::getOpenFileName(this,"DSDT","","DSDT(*.aml *.dsl *.dat);;All(*.*)");

        if (!fileName.isEmpty())
        {

            loadFile(openFile(fileName));
        }

    }
}

bool MainWindow::maybeSave()
{
    if (!textEdit->isModified())
        return true;
    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Application"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret)
    {
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
    QString fn = dialog.getSaveFileName(this,"DSDT","","DSDT(*.dsl);;All(*.*)");
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
        out << textEdit->text();
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

    return true;
}

void MainWindow::btnGenerate_clicked()
{
    QFileInfo appInfo(qApp->applicationDirPath());
    qDebug() << appInfo.filePath();

    QProcess dump;
    QProcess iasl;


#ifdef Q_OS_WIN32
// win
    dump.execute(appInfo.filePath() + "/acpidump.exe" , QStringList() << "-b");//阻塞
    iasl.execute(appInfo.filePath() + "/iasl.exe" , QStringList() << "-d" << "dsdt.dat");
#endif

#ifdef Q_OS_LINUX
// linux
    dump.execute(appInfo.filePath() + "/acpidump" , QStringList() << "-b");
    iasl.execute(appInfo.filePath() + "/iasl" , QStringList() << "-d" << "dsdt.dat");

#endif

#ifdef Q_OS_MAC
// mac
    dump.execute(appInfo.filePath() + "/acpidump" , QStringList() << "-b");
    iasl.execute(appInfo.filePath() + "/iasl" , QStringList() << "-d" << "dsdt.dat");

#endif

    loadFile(appInfo.filePath() + "/dsdt.dsl");

}

void MainWindow::btnCompile_clicked()
{
    QFileInfo cf_info(curFile);
    if(cf_info.suffix().toLower() != "dsl")
        return;

    QFileInfo appInfo(qApp->applicationDirPath());
    co = new QProcess;

    if(!curFile.isEmpty())
        btnSave_clicked();

    lblMsg->setText("Compiling...");
    qTime.start();


#ifdef Q_OS_WIN32
// win
   co->start(appInfo.filePath() + "/iasl.exe" , QStringList() << "-f" << curFile);
#endif

#ifdef Q_OS_LINUX
// linux
   co->start(appInfo.filePath() + "/iasl" , QStringList() << "-f" << curFile);

#endif

#ifdef Q_OS_MAC
// mac
    co->start(appInfo.filePath() + "/iasl" , QStringList() << "-f" << curFile);

#endif

    connect(co , SIGNAL(finished(int)) , this , SLOT(readResult(int)));

    /*仅供测试*/
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

                    highlight_cursor.mergeCharFormat(color_format);

                }
            }

           cursor.endEditBlock();

        }
}

/*读取编译结果信息*/
void MainWindow::readResult(int exitCode)
{
    loading = true;

    textEditTemp->clear();

    QString result, result2;
    /*QTextCodec* gCodec = QTextCodec::codecForName("GBK");

    if(gCodec)
    {
        result = gCodec->toUnicode(co->readAll());
        result2 = gCodec->toUnicode(co->readAllStandardError());
    }
    else
    {
        result = QString::fromUtf8(co->readAll());
        result2 = QString::fromUtf8(co->readAllStandardError());
    }*/

    result = QString::fromUtf8(co->readAll());
    result2 = QString::fromUtf8(co->readAllStandardError());

    textEditTemp->append(result);

    textEditTemp->append(result2);

    //分离基本信息
    ui->editShowMsg->clear();
    QVector<QString> list;
    for(int i = 0; i < textEditTemp->document()->lineCount(); i++)
    {
        QString str = textEditTemp->document()->findBlockByNumber(i).text();

        list.push_back(str);
        QString str_sub = str.trimmed();
        if(str_sub.mid(0, 5) == "Error" || str_sub.mid(0, 7) == "Warning" || str_sub.mid(0, 6) == "Remark")
        {
             for(int j = 0; j < i - 2; j++)
                ui->editShowMsg->append(list.at(j));

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

    //清除所有标记
    textEdit->SendScintilla(QsciScintilla::SCI_MARKERDELETEALL);

    float a = qTime.elapsed()/1000.00;
    lblMsg->setText("Compiled(" + QTime::currentTime().toString() + "    " + QString::number(a, 'f', 2) + " s)");


    if(exitCode == 0)
    {

        ui->btnPreviousError->setEnabled(false);
        ui->btnNextError->setEnabled(false);
        ui->tabWidget->setCurrentIndex(0);

        QMessageBox::information(this , "编译(Compiling)" , "编译成功(Compiled successfully.)");

    }
    else
    {
        ui->btnPreviousError->setEnabled(true);
        ui->btnNextError->setEnabled(true);
        ui->tabWidget->setCurrentIndex(1);

        on_btnNextError_clicked();
    }

    ui->tabWidget->setHidden(false);

    loading = false;

}

void MainWindow::textEdit_cursorPositionChanged()
{
    int ColNum , RowNum;
    textEdit->getCursorPosition(&RowNum , &ColNum);

    ui->statusbar->showMessage(QString::number(RowNum + 1) + "    " + QString::number(ColNum));

    //联动treeWidget
    mem_linkage(ui->treeWidget);

}

/*换行之后，2s后再刷新成员树*/
void MainWindow::timer_linkage()
{
    if(!loading)
    {
         on_btnRefreshTree_clicked();

         timer->stop();

    }

}

/*单击文本任意位置，刷新成员树，进行联动*/
void MainWindow::mem_linkage(QTreeWidget * tw)
{

    int ColNum , RowNum;
    textEdit->getCursorPosition(&RowNum , &ColNum);

    if(!loading && tw->topLevelItemCount() > 0)
    {
        int cu; //当前位置
        int cu1;//下一个位置

        for(int i = 0; i < tw->topLevelItemCount(); i++)
        {
            cu = tw->topLevelItem(i)->text(1).toInt();

            if(i + 1 == tw->topLevelItemCount())
            {
                cu1 = cu;
            }
            else
            {
                cu1 = tw->topLevelItem(i + 1)->text(1).toInt();
            }


            if(RowNum == cu)
            {

                tw->setCurrentItem(tw->topLevelItem(i));

                break;

            }
            else
            {
                if(RowNum > cu && RowNum < cu1)
                {
                    tw->setCurrentItem(tw->topLevelItem(i));
                    break;
                }

            }

         }

        //最后一个,单独处理
        int t = tw->topLevelItemCount();
        cu = tw->topLevelItem(t - 1)->text(1).toUInt();
        if(RowNum > cu)
        {

           tw->setCurrentItem(tw->topLevelItem(t - 1));

        }

    }

}

/*行号区域的宽度：目前在主编辑框内已弃用，为编译输出信息显示预留*/
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

    textEdit->replace(ui->editReplace->text());

}

void MainWindow::on_btnFindNext_clicked()
{
    QString str = ui->editFind->text().trimmed();
    //正则、大小写、匹配整个词、循环查找、向下或向上：目前已开启向下的循环查找ß
    if(textEdit->findFirst(str , true , CaseSensitive , false , true, true))
    {

        if(red < 55)
        {
            ui->editFind->setStyleSheet("background-color:rgba(50,50,50,255)");

        }
        else
        {
            ui->editFind->setStyleSheet("background-color:rgba(255,255,255,255)");

        }

    }
    else
    {
        if(str.count() > 0)
        {
            ui->editFind->setStyleSheet("background-color:rgba(255,70,70)");
            //字色
            QPalette palette;
            palette.setColor(QPalette::Text,Qt::white);
            ui->editFind->setPalette(palette);
        }
    }

    find_down = true;
    find_up = false;


}

void MainWindow::on_btnFindPrevious_clicked()
{

    QString name = ui->editFind->text().trimmed();
    std::string str = name.toStdString();
    const char* ch = str.c_str();

    int flags;
    if(CaseSensitive)
        flags = QsciScintilla::SCFIND_MATCHCASE | QsciScintilla::SCFIND_REGEXP;
    else
        flags = QsciScintilla::SCFIND_REGEXP;

    textEdit->SendScintilla(QsciScintilla::SCI_SEARCHANCHOR);
    if(textEdit->SendScintilla(QsciScintilla::SCI_SEARCHPREV, flags, ch) == -1)
    {
        //ui->editFind->setStyleSheet("background-color:rgba(255,125,125,255)");//没找到
    }
    else
    {
        if(red < 55)
        {
            ui->editFind->setStyleSheet("background-color:rgba(50,50,50,255)");

        }
        else
        {
            ui->editFind->setStyleSheet("background-color:rgba(255,255,255,255)");

        }
    }


    QScrollBar *vscrollbar = new QScrollBar;
    vscrollbar = textEdit->verticalScrollBar();

    QScrollBar *hscrollbar = new QScrollBar;
    hscrollbar = textEdit->horizontalScrollBar();

    int row, col, vs_pos, hs_pos;
    vs_pos = vscrollbar->sliderPosition();
    textEdit->getCursorPosition(&row, &col);
    if(row < vs_pos)
        vscrollbar->setSliderPosition(row - 5);

    hs_pos = hscrollbar->sliderPosition();
    QPainter p(this);
    QFontMetrics fm = p.fontMetrics();
    QString t = textEdit->text(row).mid(0, col);
    int char_w = fm.horizontalAdvance(t);//一个字符的宽度
    qDebug() << col;
    if(char_w < textEdit->viewport()->width())
        hscrollbar->setSliderPosition(0);
    else
        hscrollbar->setSliderPosition(char_w);// + fm.horizontalAdvance(name));

    qDebug() << col << textEdit->horizontalScrollBar()->sliderPosition();
    find_down = false;
    find_up = true;

}

void MainWindow::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{

    if(column == 0 && !loading)
    {
        int lines = item->text(1).toInt();
        textEdit->setCursorPosition(lines , 0);
        textEdit->setFocus();

    }

}

void MainWindow::treeWidgetBack_itemClicked(QTreeWidgetItem *item, int column)
{

    if(column == 0)
    {
        int lines = item->text(1).toInt();
        textEdit->setCursorPosition(lines , 0);
        textEdit->setFocus();

    }

}


void MainWindow::on_editShowMsg_cursorPositionChanged()
{
    set_cursor_line_color(ui->editShowMsg);
}

void MainWindow::set_cursor_line_color(QTextEdit * edit)
{
    QList<QTextEdit::ExtraSelection> extraSelection;
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(255,255,0, 50);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection,true);
    selection.cursor = edit->textCursor();
    //selection.cursor.clearSelection();
    extraSelection.append(selection);
    edit->setExtraSelections(extraSelection);
}

void MainWindow::on_btnNextError_clicked()
{

    const QTextCursor cursor = ui->editErrors->textCursor();
    int RowNum = cursor.blockNumber();

    QTextBlock block = ui->editErrors->document()->findBlockByNumber(RowNum);
    ui->editErrors->setTextCursor(QTextCursor(block));


    for(int i = RowNum + 1; i < ui->editErrors->document()->lineCount(); i++)
    {
        QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
        ui->editErrors->setTextCursor(QTextCursor(block));


        QString str = ui->editErrors->document()->findBlockByLineNumber(i).text();
        QString sub = str.trimmed();


        if(sub.mid(0 , 5) == "Error")
        {
            QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
            ui->editErrors->setTextCursor(QTextCursor(block));

            QList<QTextEdit::ExtraSelection> extraSelection;
            QTextEdit::ExtraSelection selection;
            QColor lineColor = QColor(Qt::red);
            selection.format.setForeground(Qt::white);
            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection,true);
            selection.cursor = ui->editErrors->textCursor();
            selection.cursor.clearSelection();
            extraSelection.append(selection);
            ui->editErrors->setExtraSelections(extraSelection);

            //定位到错误行
            getErrorLine(i);

            ui->tabWidget->setCurrentIndex(1);

            break;

        }

        if(i == ui->editShowMsg->document()->lineCount() - 1)
            on_btnPreviousError_clicked();
    }
}

void MainWindow::on_btnPreviousError_clicked()
{

    const QTextCursor cursor = ui->editErrors->textCursor();
    int RowNum = cursor.blockNumber();

    QTextBlock block = ui->editErrors->document()->findBlockByNumber(RowNum);
    ui->editErrors->setTextCursor(QTextCursor(block));


    for(int i = RowNum - 1; i > -1; i--)
    {
        QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
        ui->editErrors->setTextCursor(QTextCursor(block));


        QString str = ui->editErrors->document()->findBlockByLineNumber(i).text();
        QString sub = str.trimmed();

        if(sub.mid(0 , 5) == "Error")
        {
            QTextBlock block = ui->editErrors->document()->findBlockByNumber(i);
            ui->editErrors->setTextCursor(QTextCursor(block));

            QList<QTextEdit::ExtraSelection> extraSelection;
            QTextEdit::ExtraSelection selection;
            //QColor lineColor = QColor(Qt::gray).lighter(150);
            QColor lineColor = QColor(Qt::red);
            selection.format.setForeground(Qt::white);
            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection,true);
            selection.cursor = ui->editErrors->textCursor();
            selection.cursor.clearSelection();
            extraSelection.append(selection);
            ui->editErrors->setExtraSelections(extraSelection);

            //定位到错误行
            getErrorLine(i);

            ui->tabWidget->setCurrentIndex(1);

            break;

        }

        if(i == 0)
            on_btnNextError_clicked();
    }

}

void MainWindow::gotoLine(QTextEdit *edit)
{
    QString text, str2, str3;
    int line = 0;
    bool skip = true;
    const QTextCursor cursor = edit->textCursor();
    int RowNum = cursor.blockNumber();

    text = edit->document()->findBlockByNumber(RowNum).text().trimmed();

    if(text != "")
    {
        for(int j = 3; j < text.count(); j++)
        {

            if(text.mid(j , 1) == ":")
            {
                str2 = text.mid(0 , j);
                skip = false;
                break;
            }

        }

        if(skip)
        {
            //再看看上一行
            text = edit->document()->findBlockByNumber(RowNum - 1).text().trimmed();
            if(text != "")
            {
                for(int j = 3; j < text.count(); j++)
                {

                    if(text.mid(j , 1) == ":")
                    {
                        str2 = text.mid(0 , j);

                        break;
                    }
                }

            }
        }


        for(int k = str2.count(); k > 0; k--)
        {
            if(str2.mid(k - 1 , 1) == " ")
            {
                str3 = str2.mid(k , str2.count() - k);

                //定位到错误行
                line = str3.toInt();
                textEdit->setCursorPosition(line - 1 , 0);

                textEdit->setFocus();

                break;
            }
        }
    }

}

void MainWindow::getErrorLine(int i)
{
    //定位到错误行
    QString str1 = ui->editErrors->document()->findBlockByLineNumber(i - 1).text().trimmed();
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
                textEdit->setCursorPosition(str3.toInt() - 1 , 0);
                int linenr = str3.toInt();
                //SCI_MARKERGET 参数用来设置标记，默认为圆形标记
                textEdit->SendScintilla(QsciScintilla::SCI_MARKERGET, linenr - 1);
                //SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
                textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0, QColor(Qt::red));
                textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0, QColor(Qt::red));
                textEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, linenr - 1);
                //下划线
                //textEdit->SendScintilla(QsciScintilla::SCI_STYLESETUNDERLINE,linenr,true);
                //textEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE,0,QsciScintilla::SC_MARK_UNDERLINE);

                textEdit->setFocus();

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

        textEdit->setCursorPosition(row_num - 1 , 0);

        textEdit->setFocus();

    }
}

void MainWindow::textEdit_textChanged()
{
    if(!loading)
    {

    }
}

void MainWindow::on_editFind_returnPressed()
{

    on_btnFindNext_clicked();
}

const char * QscilexerCppAttach::keywords(int set) const
{
    //if(set == 1 || set == 3)
    //    return QsciLexerCPP::keywords(set);

    if (set == 1)
            return
                "and and_eq asm auto bitand bitor bool break case "
                "catch char class compl const const_cast continue "
                "default delete do double dynamic_cast else enum "
                "explicit export extern false float for friend goto if "
                "inline int long mutable namespace new not not_eq "
                "operator or or_eq private protected public register "
                "reinterpret_cast return short signed sizeof static "
                "static_cast struct switch template this throw true "
                "try typedef typeid typename union unsigned using "
                "virtual void volatile wchar_t while xor xor_eq "
                ""

                "External Scope Device Method Name If While Break Return ElseIf Switch Case Else "
                "Default Field OperationRegion Package DefinitionBlock Offset CreateDWordField CreateByteField "
                "CreateBitField CreateWordField CreateQWordField Buffer ToInteger ToString ToUUID ToUuid ToHexString ToDecimalString ToBuffer ToBcd"
                "CondRefOf FindSetLeftBit FindSetRightBit FromBcd Function CreateField "

                "Acquire Add Alias And "
                "BankField AccessAs CondRefOf ExtendedMemory ExtendedSpace "
                "BreakPoint Concatenate ConcatenateResTemplate Connection Continue CopyObject DataTableRegion Debug Decrement DerefOf "
                "Divide Dma Arg0 Arg1 Arg2 Arg3 Arg4 Arg5 Arg6 "
                "DWordIo EisaId EndDependentFn Event ExtendedIo Fatal FixedDma FixedIo GpioInt GpioIo "
                "Increment Index IndexField Interrupt Io Irq IrqNoFlags "
                "LAnd LEqual LGreater LGreaterEqual LLess LLessEqual LNot LNotEqual Load LOr Match Mid Mod Multiply "
                "Mutex NAnd NoOp NOr Not Notify ObjectType Or PowerResource Revision "
                "Memory32Fixed "
                "DWordMemory Local0 Local1 Local2 Local3 Local4 Local5 Local6 Local7 "
                "DWordSpace One Ones Processor QWordIo Memory24 Memory32 VendorLong VendorShort Wait WordBusNumber WordIo WordSpace "
                "I2cSerialBusV2 Include LoadTable QWordMemory QWordSpace RawDataBuffer RefOf Register Release Reset ResourceTemplate ShiftLeft ShiftRight Signal SizeOf Sleep "
                "SpiSerialBusV2 Stall StartDependentFn StartDependentFnNoPri Store Subtract ThermalZone Timer ToBcd UartSerialBusV2 Unicode Unload "
                "Xor Zero ";


    if (set == 2)
            return
                 "SubDecode PosDecode AttribBytes SubDecode PosDecode ReadWrite ReadOnly Width8bit Width16bit Width32bit Width64bit Width128bit Width256bit "
                 "UserDefRegionSpace SystemIO SystemMemory TypeTranslation TypeStatic AttribRawBytes AttribRawProcessBytes Serialized NotSerialized "
                 "key dict array TypeA TypeB TypeF AnyAcc ByteAcc Cacheable WriteCombining Prefetchable NonCacheable PullDefault PullUp PullDown PullNone "
                 "MethodObj UnknownObj IntObj DeviceObj MutexObj PkgObj FieldUnitObj StrObj Edge Level ActiveHigh ActiveLow ActiveBoth "
                 "BuffObj EventObj OpRegionObj PowerResObj ProcessorObj ThermalZoneObj BuffFieldObj DDBHandleObj None ReturnArg PolarityHigh PolarityLow ThreeWireMode FourWireMode "
                 "MinFixed MinNotFixed MaxFixed MaxNotFixed ResourceConsumer ResourceProducer MinFixed MinNotFixed MaxFixed MaxNotFixed ClockPolarityLow ClockPolarityHigh "
                 "ResourceConsumer ResourceProducer SubDecode PosDecode MaxFixed MaxNotFixed GeneralPurposeIo GenericSerialBus FFixedHW ClockPhaseFirst ClockPhaseSecond "
                 "MTR MEQ MLE MLT MGE MGT WordAcc DWordAcc QWordAcc BufferAcc Lock NoLock AddressRangeMemory AddressRangeReserved AddressRangeNVS AddressRangeACPI FlowControlHardware "
                 "AttribQuick AttribSendReceive AttribByte AttribWord AttribBlock AttribProcessCall AttribBlockProcessCall IoRestrictionNone IoRestrictionInputOnly IoRestrictionOutputOnly IoRestrictionNoneAndPreserve "
                 "Preserve WriteAsOnes WriteAsZeros Compatibility BusMaster NotBusMaster Transfer8 Transfer16 Transfer8_16 DataBitsFive DataBitsSix DataBitsSeven ParityTypeOdd ParityTypeEven FlowControlNone FlowControlXon "
                 "ResourceConsumer ResourceProducer SubDecode PosDecode MinFixed MinNotFixed PCI_Config EmbeddedControl SMBus SystemCMOS PciBarTarget IPMI BigEndian LittleEndian ParityTypeNone ParityTypeSpace ParityTypeMark "
                 "ISAOnlyRanges NonISAOnlyRanges EntireRange TypeTranslation TypeStatic SparseTranslation DenseTranslation DataBitsEight DataBitsNine StopBitsZero StopBitsOne StopBitsOnePlusHalf StopBitsTwo "
                 "Exclusive SharedAndWake ExclusiveAndWake Shared ControllerInitiated DeviceInitiated AddressingMode7Bit AddressingMode10Bit Decode16 Decode10 ";


    if (set == 3)
            return
                "a addindex addtogroup anchor arg attention author b "
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


QString findKey(QString str, QString str_sub, int f_null)
{
    int total , tab_count;
    QString strs , space;
    tab_count = 0;
    for(int i = 0; i < str.count(); i++)
    {
        if(str.mid(i, 1) == str_sub)
        {
            strs = str.mid(0, i);

            for(int j = 0; j < strs.count(); j++)
            {
                if(strs.mid(j, 1) == "\t")
                {
                    tab_count = tab_count + 1;

                }
                //qDebug() <<"\t个数：" << strs.mid(j, 1) << tab_count;
            }

            int str_space =  strs.count() - tab_count;
            total = str_space + tab_count * 4 - f_null;

            for(int k = 0; k < total; k++)
                space = space + " ";

            break;

        }
    }

    return space;
}

void MainWindow::textEdit_linesChanged()
{
    if(!loading)
       timer->start(2000);

}

void thread_one::run()
{

    //在线程中刷新成员列表
    //QMetaObject::invokeMethod(this->parent(),"refreshTree");

    if(break_run)
    {
        thread_end = true;
        return;
    }

    thread_end = false;

    refreshTree();

    emit over();  //发送完成信号
}

void MainWindow::dealover()//收到线程结束信号
{

    if(break_run)
    {
        thread_end = true;
        return;
    }

    update_ui_tw();

    thread_end = true;


}

void MainWindow::update_member(bool show, QString str_void, QList<QTreeWidgetItem *> tw_list)
{
    if(!show)
    {

        tw_list.clear();
        for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
        {
            QString str = ui->treeWidget->topLevelItem(i)->text(0).trimmed();
            if(str.mid(0 , str_void.count()) == str_void)
            {
                tw_list.append(ui->treeWidget->takeTopLevelItem(i));
                i = -1;

            }
        }
    }
    else
    {
        //if(tw_list.count() > 0)
        //{
            ui->treeWidget->addTopLevelItems(tw_list);
            ui->treeWidget->sortItems(1 , Qt::AscendingOrder);

        //}
        //else
        //    on_btnRefreshTree_clicked();

        qDebug() << tw_list.count();

    }


}

void MainWindow::update_ui_tw()
{
    ui->treeWidget->clear();

    ui->treeWidget->update();

    ui->treeWidget->addTopLevelItems(twitems);

    ui->treeWidget->sortItems(1 , Qt::AscendingOrder);//排序

    ui->treeWidget->setHeaderLabel("S(" + QString::number(s_count) + ")  " + "D(" + QString::number(d_count) + ")  " + "M(" + QString::number(m_count) + ")  "  + "N(" + QString::number(n_count) + ")");
    ui->treeWidget->update();

    float a = qTime.elapsed()/1000.00;
    lblMsg->setText("Refresh completed(" + QTime::currentTime().toString() + "    " + QString::number(a, 'f', 2) + " s)");

    textEdit_cursorPositionChanged();

}

void MainWindow::on_btnRefreshTree_clicked()
{

    if(!thread_end)
    {
        break_run = true;
        lblMsg->setText("Refresh interrupted");
        mythread->quit();
        mythread->wait();
        return;
    }
    else
        break_run = false;

    //将textEdit的内容读到后台
    textEditBack->clear();
    textEditBack->setText(textEdit->text());
    lblMsg->setText("Refreshing...");
    qTime.start();
    mythread->start();

}

QString getMemberName(QString str_member, QsciScintilla *textEdit)
{
    int RowNum, ColNum;
    QString str;
    textEdit->getCursorPosition(&RowNum, &ColNum);
    str = textEdit->text(RowNum);

    //QString space = findKey(str, str_member.mid(0, 1), 4);

    QString sub = str.trimmed();

    bool zs = false;//当前行是否存在注释
    for(int k = ColNum; k > -1; k--)
    {
        if(str.mid(k - 2 , 2) == "//" || str.mid(k - 2 , 2) == "/*")
        {
            zs = true;

            break;
        }
    }

    QString str_end;
    if(sub.mid(0 , str_member.count()) == str_member && !zs)
    {

        for(int i = 0; i < sub.count(); i++)
        {
            if(sub.mid(i , 1) == ")")
            {
                str_end = sub.mid(0 , i + 1);
                break;

            }
        }
    }

    return str_end;

}

void MainWindow::getMemberTree()
{
    ui->treeWidget->clear();
    ui->treeWidget->repaint();

    QString str_member = "Scope";
    QString str;
    int RowNum, ColNum;
    int count;  //总行数
    int dkh1 = 0;
    int dkh2 = 0;
    int c_fw = 0; //当前范围
    QTreeWidgetItem *twItem0;

    count = textEdit->lines();

    //回到第一行
    textEdit->setCursorPosition(0, 0);

    for(int j = 0; j < count; j++)
    {

        //正则、区分大小写、匹配整个单词、循环搜索
        if(textEdit->findFirst(str_member , true , true , true , false))
        {

           textEdit->getCursorPosition(&RowNum, &ColNum);
           twItem0 = new QTreeWidgetItem(QStringList() << getMemberName(str_member, textEdit) << QString("%1").arg(RowNum, 7, 10, QChar('0')));
           twItem0->setIcon(0, QIcon(":/icon/s.png"));
           ui->treeWidget->addTopLevelItem(twItem0);

           qDebug() << getMemberName(str_member, textEdit);

           //再往下找内部成员

           int c_row = RowNum;

           for(int s = c_row; s < count; s++)
           {
               if(textEdit->findFirst(str_member , true , true , true , false))
               {
                   //范围结束
                   int row, col;
                   textEdit->getCursorPosition(&row, &col);
                   c_fw = row;

                   qDebug() << "范围结束" << c_fw;
                   break;
               }
               else
               {
                   textEdit->setCursorPosition(c_row, 0);
                   dkh1 = 1;
                   for(int d1 = c_row; d1 < count; d1++)
                   {
                       if(textEdit->findFirst("{" , true , true , true , false))
                       {

                           dkh1 ++;

                       }


                       if(textEdit->findFirst("}" , true , true , true , false))
                       {
                           dkh1 --;

                       }


                       qDebug() << dkh1 << dkh2;

                       if(dkh1 == 0)
                       {
                           //范围结束
                           int row, col;
                           textEdit->getCursorPosition(&row, &col);
                           c_fw = row + 1;

                           qDebug() << "范围结束" << c_fw;
                           //break;

                       }

                   }
               }

           }



           textEdit->setCursorPosition(c_row, 0);
           for(int d = c_row; d < c_fw; d++)
           {

                   if(textEdit->findFirst("Device" , true , true , true , false))
                   {

                       textEdit->getCursorPosition(&RowNum, &ColNum);
                       QTreeWidgetItem *twItem1 = new QTreeWidgetItem(QStringList() << getMemberName("Device", textEdit) << QString("%1").arg(RowNum, 7, 10, QChar('0')));
                       twItem1->setIcon(0, QIcon(":/icon/d.png"));
                       twItem0->addChild(twItem1);

                   }

            }
           textEdit->setCursorPosition(c_row, 0);
           for(int m = c_row; m < c_fw; m++)
           {

                   if(textEdit->findFirst("Method" , true , true , true , false))
                   {

                       textEdit->getCursorPosition(&RowNum, &ColNum);
                       QTreeWidgetItem *twItem1 = new QTreeWidgetItem(QStringList() << getMemberName("Method", textEdit) << QString("%1").arg(RowNum, 7, 10, QChar('0')));
                       twItem1->setIcon(0, QIcon(":/icon/m.png"));
                       twItem0->addChild(twItem1);
                   }

            }

           //枚举完成
           textEdit->setCursorPosition(c_fw, 0);
           //j = c_fw - 1;
            qDebug() << j;

         }
        else
            break;

    }

    ui->treeWidget->sortItems(1 , Qt::AscendingOrder);//排序

    ui->treeWidget->expandAll();

}

void refreshTree()
{
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

void getMembers(QString str_member, QsciScintilla *textEdit)
{

    if(break_run)
        return;

    QString str;
    int RowNum, ColNum;
    int count;  //总行数
    QTreeWidgetItem *twItem0;

    count = textEdit->lines();

    //回到第一行
    textEdit->setCursorPosition(0, 0);

    for(int j = 0; j < count; j++)
    {
        if(break_run)
        {
            //qDebug() << "breaked  " + str_member + "  " + QTime::currentTime().toString();
            break;
        }

        //正则、区分大小写、匹配整个单词、循环搜索
        if(textEdit->findFirst(str_member , true , true , true , false))
        {

           textEdit->getCursorPosition(&RowNum, &ColNum);

           str = textEdit->text(RowNum);

           QString space = findKey(str, str_member.mid(0, 1), 4);

           QString sub = str.trimmed();

           bool zs = false;//当前行是否存在注释
           for(int k = ColNum; k > -1; k--)
           {
               if(str.mid(k - 2 , 2) == "//" || str.mid(k - 2 , 2) == "/*")
               {
                   zs = true;

                   break;
               }
           }

           QString str_end;
           if(sub.mid(0 , str_member.count()) == str_member && !zs)
           {

               for(int i = 0; i < sub.count(); i++)
               {
                   if(sub.mid(i , 1) == ")")
                   {
                       str_end = sub.mid(0 , i + 1);

                       twItem0 = new QTreeWidgetItem(QStringList() << space + str_end << QString("%1").arg(RowNum, 7, 10, QChar('0')));//QString::number(RowNum));

                       if(str_member == "Scope" && show_s)
                       {

                           twItem0->setIcon(0, QIcon(":/icon/s.png"));

                           twitems.append(twItem0);

                           s_count++;
                       }
                       if(str_member == "Method" && show_m)
                       {

                           twItem0->setIcon(0, QIcon(":/icon/m.png"));

                           twitems.append(twItem0);

                           m_count++;

                       }
                       if(str_member == "Name" && show_n)
                       {

                           twItem0->setIcon(0, QIcon(":/icon/n.png"));

                           twitems.append(twItem0);

                           n_count++;

                       }
                       if(str_member == "Device" && show_d)
                       {

                           twItem0->setIcon(0, QIcon(":/icon/d.png"));

                           twitems.append(twItem0);

                           d_count++;

                       }

                       break;

                   }
               }


           }
         }
        else
            break;

    }

}

void MainWindow::on_MainWindow_destroyed()
{


}

void MainWindow::init_info_edit()
{


    textEditTemp = new QTextEdit();

    ui->tabWidget->setMaximumHeight(250);
    ui->tabWidget->setMinimumHeight(150);

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
    ui->tabWidget->removeTab(4);//暂时不用"优化"这项


}

void MainWindow::init_menu()
{

    ui->menu_File->addAction(ui->actionNew);
    ui->menu_File->addAction(ui->actionOpen);
    ui->menu_File->addAction(ui->actionSave);
    ui->menu_File->addAction(ui->actionSaveAs);
    ui->menu_File->addSeparator();
    ui->menu_File->addSeparator();
    ui->menu_File->addAction(ui->actionAbout);

    ui->actionNew->setShortcut(tr("ctrl+n"));
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);

    ui->actionOpen->setShortcut(tr("ctrl+o"));
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::btnOpen_clicked);

    ui->actionSave->setShortcut(tr("ctrl+s"));
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::btnSave_clicked);

    ui->actionSaveAs->setShortcut(tr("ctrl+shift+s"));
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::btnSaveAs_clicked);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

    ui->actionGenerate->setShortcut(tr("ctrl+g"));
    connect(ui->actionGenerate, &QAction::triggered, this, &MainWindow::btnGenerate_clicked);

    ui->actionCompiling->setShortcut(tr("ctrl+m"));
    connect(ui->actionCompiling, &QAction::triggered, this, &MainWindow::btnCompile_clicked);

    ui->actionFont_2->setShortcut(tr("ctrl+f"));
    connect(ui->actionFont_2, &QAction::triggered, this, &MainWindow::set_font);

    ui->actionWrapWord->setShortcut(tr("ctrl+w"));
    connect(ui->actionWrapWord, &QAction::triggered, this, &MainWindow::set_wrap);


}

void MainWindow::setLexer(QsciLexer *textLexer)
{


    //获取背景色
    QPalette pal = this->palette();
    QBrush brush = pal.window();
    red = brush.color().red();

    if(red < 55) //暗模式，mac下为50
    {

        //设置光标所在行背景色
        textEdit->setCaretLineBackgroundColor(QColor(180, 180, 0));
        textEdit->setCaretLineFrameWidth(1);
        textEdit->setCaretLineVisible(true);

        //背景色
        textLexer->setPaper(QColor(28, 28, 28));

        ui->editFind->setStyleSheet("background-color:rgba(28,28,28,255)");

        //textLexer->setColor(QColor(255,255,255, 255), -1);

        textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::CommentLine);//"//"注释颜色
        textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::Comment);

        textLexer->setColor(QColor(210, 210, 210 ), QsciLexerCPP::Identifier);
        textLexer->setColor(QColor(245, 150, 147), QsciLexerCPP::Number);
        textLexer->setColor(QColor(100, 100, 250), QsciLexerCPP::Keyword);
        textLexer->setColor(QColor(210, 32, 240 ), QsciLexerCPP::KeywordSet2);
        textLexer->setColor(QColor(245, 245, 245 ), QsciLexerCPP::Operator);
        textLexer->setColor(QColor(84, 255, 159 ), QsciLexerCPP::DoubleQuotedString);//双引号

    }
    else
    {

        //背景色
        textLexer->setPaper(QColor(255, 255, 255));

        ui->editFind->setStyleSheet("background-color:rgba(255,255,255,255)");

        textEdit->setCaretLineBackgroundColor(QColor(255, 255, 0, 50));
        textEdit->setCaretLineFrameWidth(0);
        textEdit->setCaretLineVisible(true);

        textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::CommentLine);//"//"注释颜色
        textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::Comment);

        textLexer->setColor(QColor(255, 0, 0), QsciLexerCPP::Number);
        textLexer->setColor(QColor(0, 0, 255), QsciLexerCPP::Keyword);
        textLexer->setColor(QColor(0, 0, 0 ), QsciLexerCPP::Identifier);
        textLexer->setColor(QColor(210, 0, 210), QsciLexerCPP::KeywordSet2);
        textLexer->setColor(QColor(20, 20, 20 ), QsciLexerCPP::Operator);
        textLexer->setColor(QColor(205, 38, 38), QsciLexerCPP::DoubleQuotedString);//双引号

    }

    //QFont font1;
    //font1.setBold(true);
    //textLexer->setFont(font1, QsciLexerCPP::KeywordSet2);

    //设置行号栏宽度、颜色
    textEdit->setMarginWidth(0, 70);
    textEdit->setMarginLineNumbers(0, true);
    if(red < 55) //暗模式，mac下为50
    {
        textEdit->setMarginsBackgroundColor(QColor(50, 50 ,50));
        textEdit->setMarginsForegroundColor(Qt::white);

    }
    else
    {
        textEdit->setMarginsBackgroundColor(brush.color());
        textEdit->setMarginsForegroundColor(Qt::black);
    }

    //匹配大小括弧
    textEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    //textEdit->setBraceMatching(QsciScintilla::StrictBraceMatch));//不推荐
    if(red > 55) //亮模式，mac下阈值为50
    {
        textEdit->setMatchedBraceBackgroundColor(QColor(Qt::green));
        textEdit->setMatchedBraceForegroundColor(QColor(Qt::red));
    }


    //设置括号等自动补全
    textEdit->setAutoIndent(true);
    textEdit->setTabIndents(true);//true如果行前空格数少于tabWidth，补齐空格数,false如果在文字前tab同true，如果在行首tab，则直接增加tabwidth个空格

    //代码提示
    QsciAPIs *apis = new QsciAPIs(textLexer);
    if(apis->load(":/data/apis.txt"))
    {

    }
    else
        apis->add(QString("Device"));

    apis->prepare();


    //设置自动补全
    textEdit->setCaretLineVisible(true);
    // Ascii|None|All|Document|APIs
    //禁用自动补全提示功能、所有可用的资源、当前文档中出现的名称都自动补全提示、使用QsciAPIs类加入的名称都自动补全提示
    textEdit->setAutoCompletionSource(QsciScintilla::AcsAll);//自动补全,对于所有Ascii字符
    textEdit->setAutoCompletionCaseSensitivity(false);//大小写敏感度
    textEdit->setAutoCompletionThreshold(2);//从第几个字符开始出现自动补全的提示
    //textEdit->setAutoCompletionReplaceWord(false);//是否用补全的字符串替代光标右边的字符串

    //设置缩进参考线
    textEdit->setIndentationGuides(true);
    //textEdit->setIndentationGuidesBackgroundColor(QColor(Qt::white));
    //textEdit->setIndentationGuidesForegroundColor(QColor(Qt::red));

    //设置光标颜色
    if(red < 55) //暗模式，mac下为50
        textEdit->setCaretForegroundColor(QColor(Qt::white));
    else
        textEdit->setCaretForegroundColor(QColor(Qt::black));
    textEdit->setCaretWidth(2);

    //自动折叠区域
    textEdit->setMarginType(3, QsciScintilla::SymbolMargin);
    textEdit->setMarginLineNumbers(3, false);
    textEdit->setMarginWidth(3, 15);
    textEdit->setMarginSensitivity(3, true);
    textEdit->setFolding(QsciScintilla::BoxedTreeFoldStyle);//折叠样式
    if(red < 55) //暗模式，mac下为50
    {
        textEdit->setFoldMarginColors(Qt::gray, Qt::black);
        //textEdit->setMarginsForegroundColor(Qt::red);  //行号颜色
        textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDFLAGS, 16);//设置折叠标志
        //textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDMARGINCOLOUR,Qt::red);
    }
    else
    {
        textEdit->setFoldMarginColors(Qt::gray, Qt::white);//折叠栏颜色
        //textEdit->setMarginsForegroundColor(Qt::blue); //行号颜色
        textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDFLAGS, 16);//设置折叠标志
    }

}

void MainWindow::init_edit()
{

    textEdit = new QsciScintilla(this);
    textEditBack = new QsciScintilla();

    textEdit->setWrapMode(QsciScintilla::WrapNone);//不自动换行
    //设置编码为UTF-8
    textEdit->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,QsciScintilla::SC_CP_UTF8);

    textEdit->setTabWidth(4);

    //水平滚动条
    textEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTH, textEdit->viewport()->width());
    textEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTHTRACKING, true);

    connect(textEdit, &QsciScintilla::cursorPositionChanged, this, &MainWindow::textEdit_cursorPositionChanged);
    connect(textEdit, &QsciScintilla::textChanged, this, &MainWindow::textEdit_textChanged);
    connect(textEdit, &QsciScintilla::linesChanged, this, &MainWindow::textEdit_linesChanged);

    textEdit->setFont(font);
    textEdit->setMarginsFont(font);

    textLexer = new QscilexerCppAttach;
    textEdit->setLexer(textLexer);

    //读取字体
    QString qfile = QDir::homePath() + "/QtiASL.ini";
    QFile file(qfile);
    QFileInfo fi(qfile);

    if(fi.exists())
    {
        //QSettings Reg(qfile, QSettings::NativeFormat);
        QSettings Reg(qfile, QSettings::IniFormat);//全平台都采用ini格式
        font.setFamily(Reg.value("FontName").toString());
        font.setPointSize(Reg.value("FontSize").toInt());
        font.setBold(Reg.value("FontBold").toBool());
        font.setItalic(Reg.value("FontItalic").toBool());
        font.setUnderline(Reg.value("FontUnderline").toBool());

    }
    textLexer->setFont(font);

    setLexer(textLexer);

    //接受文件拖放打开
    this->textEdit->setAcceptDrops(false);
    this->setAcceptDrops(true);

    ui->editFind->setClearButtonEnabled(true);
    ui->editReplace->setClearButtonEnabled(true);


}

void MainWindow::init_treeWidget(QTreeWidget *treeWidgetBack, int w)
{

    //connect(treeWidgetBack, &QTreeWidget::itemClicked, this, &MainWindow::treeWidgetBack_itemClicked);

    treeWidgetBack->setColumnCount(2);
    treeWidgetBack->setMinimumWidth(300);
    treeWidgetBack->setMaximumWidth(w/3 - 50);
    treeWidgetBack->setColumnWidth(0 , 200);
    treeWidgetBack->setColumnWidth(1 , 100);
    treeWidgetBack->setHeaderItem(new QTreeWidgetItem(QStringList() << "Members" << "Lines"));
    //设置水平滚动条
    treeWidgetBack->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeWidgetBack->header()->setStretchLastSection(false);

    treeWidgetBack->setColumnHidden(1 , true);

    treeWidgetBack->setFont(font);

}


void MainWindow::on_chkScope_clicked()
{

    if(ui->chkScope->isChecked())
        show_s = true;
    else
        show_s = false;


    if(!show_s)
    {

        int count = ui->treeWidget->topLevelItemCount();
        tw_scope.clear();
        for(int i = 0; i < count; i++)
        {
            //重要，否则刷新非常慢
            ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(count - 1));

            QString str = ui->treeWidget->topLevelItem(i)->text(0).trimmed();
            if(str.mid(0 , 4) == "Scop")
            {
                tw_scope.append(ui->treeWidget->takeTopLevelItem(i));

                count--;
                i = i - 1;

            }

        }
    }
    else
    {
        if(tw_scope.count() == s_count && s_count != 0)
        {
            ui->treeWidget->addTopLevelItems(tw_scope);
            ui->treeWidget->sortItems(1 , Qt::AscendingOrder);

        }

        if(tw_scope.count() != s_count || s_count == 0)
            on_btnRefreshTree_clicked();



    }
    ui->treeWidget->update();


    textEdit_cursorPositionChanged();


}

void MainWindow::on_chkDevice_clicked()
{
    ui->treeWidget->update();

    if(ui->chkDevice->isChecked())
        show_d = true;
    else
        show_d = false;

    if(!show_d)
    {

        int count = ui->treeWidget->topLevelItemCount();
        tw_device.clear();
        for(int i = 0; i < count; i++)
        {
            ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(count - 1));

            QString str = ui->treeWidget->topLevelItem(i)->text(0).trimmed();
            if(str.mid(0 , 4) == "Devi")
            {
                tw_device.append(ui->treeWidget->takeTopLevelItem(i));
                ui->treeWidget->update();
                count--;
                i = i - 1;

            }


        }
    }
    else
    {
        if(tw_device.count() == d_count && d_count > 0)
        {
            ui->treeWidget->addTopLevelItems(tw_device);
            ui->treeWidget->sortItems(1 , Qt::AscendingOrder);

        }
        if(tw_device.count() != d_count || d_count == 0)
            on_btnRefreshTree_clicked();

    }
    ui->treeWidget->update();
    textEdit_cursorPositionChanged();

}

void MainWindow::on_chkMethod_clicked()
{
    ui->treeWidget->update();

    if(ui->chkMethod->isChecked())
        show_m = true;
    else
        show_m = false;

    if(!show_m)
    {

        int count = ui->treeWidget->topLevelItemCount();
        tw_method.clear();
        for(int i = 0; i < count; i++)
        {
            ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(count - 1));

            QString str = ui->treeWidget->topLevelItem(i)->text(0).trimmed();
            if(str.mid(0 , 4) == "Meth")
            {
                tw_method.append(ui->treeWidget->takeTopLevelItem(i));

                count--;
                i = i - 1;

            }


        }
    }
    else
    {
        if(tw_method.count() == m_count && m_count > 0)
        {
            ui->treeWidget->addTopLevelItems(tw_method);
            ui->treeWidget->sortItems(1 , Qt::AscendingOrder);

        }
        if(tw_method.count() != m_count || m_count == 0)
            on_btnRefreshTree_clicked();

    }
    ui->treeWidget->update();
    textEdit_cursorPositionChanged();

}

void MainWindow::on_chkName_clicked()
{
    ui->treeWidget->update();

    if(ui->chkName->isChecked())
        show_n = true;
    else
        show_n = false;

    if(!show_n)
    {

        int count = ui->treeWidget->topLevelItemCount();
        tw_name.clear();
        for(int i = 0; i < count; i++)
        {
            ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(count - 1));

            QString str = ui->treeWidget->topLevelItem(i)->text(0).trimmed();
            if(str.mid(0 , 4) == "Name")
            {
                tw_name.append(ui->treeWidget->takeTopLevelItem(i));
                ui->treeWidget->update();
                count--;
                i = i - 1;

            }


        }
    }
    else
    {
        if(tw_name.count() == n_count && n_count > 0)
        {
            ui->treeWidget->addTopLevelItems(tw_name);
            ui->treeWidget->sortItems(1 , Qt::AscendingOrder);

        }
        if(tw_name.count() != n_count || n_count == 0)
            on_btnRefreshTree_clicked();

    }
    ui->treeWidget->update();
    textEdit_cursorPositionChanged();

}

void MainWindow::separ_info(QString str_key, QTextEdit *editInfo)
{

    editInfo->clear();

    QTextBlock block = textEditTemp->document()->findBlockByNumber(0);
    textEditTemp->setTextCursor(QTextCursor(block));

    int info_count = 0;
    for(int i = 0; i < textEditTemp->document()->lineCount(); i++)
    {
        QTextBlock block = textEditTemp->document()->findBlockByNumber(i);
        textEditTemp->setTextCursor(QTextCursor(block));

        QString str = textEditTemp->document()->findBlockByLineNumber(i).text();
        QString sub = str.trimmed();


        if(sub.mid(0 , str_key.count()) == str_key)
        {

            QString str0 = textEditTemp->document()->findBlockByNumber(i - 1).text();
            editInfo->append(str0);

            editInfo->append(str);
            editInfo->append("");

            info_count++;


        }

    }

    //标记tab头
    if(str_key == "Error")
        ui->tabWidget->setTabText(1, "Errors (" + QString::number(info_count) +")");
    if(str_key == "Warning")
        ui->tabWidget->setTabText(2, "Warnings (" + QString::number(info_count) +")");
    if(str_key == "Remark")
        ui->tabWidget->setTabText(3, "Remarks (" + QString::number(info_count) +")");
    if(str_key == "Optimization")
        ui->tabWidget->setTabText(4, "Optimizations (" + QString::number(info_count) +")");

}

void MainWindow::on_editErrors_cursorPositionChanged()
{
   if(!loading)
   {
       set_cursor_line_color(ui->editErrors);
       gotoLine(ui->editErrors);
   }
}

void MainWindow::on_editWarnings_cursorPositionChanged()
{
    if(!loading)
    {
        set_cursor_line_color(ui->editWarnings);
        gotoLine(ui->editWarnings);
    }

}

void MainWindow::on_editRemarks_cursorPositionChanged()
{
    if(!loading)
    {
        set_cursor_line_color(ui->editRemarks);
        gotoLine(ui->editRemarks);
    }

}

void MainWindow::on_editOptimizations_cursorPositionChanged()
{
    set_cursor_line_color(ui->editOptimizations);
    gotoLine(ui->editOptimizations);
}

void MainWindow::regACPI_win()
{
        QString appPath = qApp->applicationFilePath();

        QString dir = qApp->applicationDirPath();
        //注意路径的替换
        appPath.replace("/", "\\");
        QString type = "QtiASL";
        QSettings *regType = new QSettings("HKEY_CLASSES_ROOT\\.dsl", QSettings::NativeFormat);
        QSettings *regIcon = new QSettings("HKEY_CLASSES_ROOT\\.dsl\\DefaultIcon", QSettings::NativeFormat);
        QSettings *regShell = new QSettings("HKEY_CLASSES_ROOT\\QtiASL\\shell\\open\\command", QSettings::NativeFormat);

        QSettings *regType1 = new QSettings("HKEY_CLASSES_ROOT\\.aml", QSettings::NativeFormat);
        QSettings *regIcon1 = new QSettings("HKEY_CLASSES_ROOT\\.aml\\DefaultIcon", QSettings::NativeFormat);
        QSettings *regShell1 = new QSettings("HKEY_CLASSES_ROOT\\QtiASL\\shell\\open\\command", QSettings::NativeFormat);

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

void MainWindow::closeEvent(QCloseEvent *event)
{

    if(textEdit->isModified())
    {
        QMessageBox message;
        message.setText("The document has been modified.");
        message.setInformativeText("Do you want to save your changes?");
        message.setStandardButtons(QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
        message.setDefaultButton(QMessageBox::Save);

        int choice = message.exec();
        switch (choice) {
            case QMessageBox::Save:
            btnSave_clicked();
            event->accept();
            break;
        case QMessageBox::Discard:
            event->accept();
            break;
        case QMessageBox::Cancel:
            event->ignore();
            break;
        }
    }
    else
    {
    event->accept();
    }
    //多窗口中，关闭窗体，删除自己
    for(int i = 0; i < wdlist.count(); i++)
    {
        if(this == wdlist.at(i))
        {
            wdlist.removeAt(i);
            filelist.removeAt(i);
        }
    }

    //关闭线程
    if(!thread_end)
    {
        mythread->quit();
        mythread->wait();
    }


}

void MainWindow::recentOpen(QString filename)
{
    if(!thread_end)
    {
        mythread->quit();
        mythread->wait();
    }

    if (maybeSave())
        loadFile(openFile(filename));
}

void MainWindow::dragEnterEvent (QDragEnterEvent *e)
{

    if (e->mimeData()->hasFormat("text/uri-list")) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent (QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    QString fileName = urls.first().toLocalFile();
    if (fileName.isEmpty()) {
        return;
    }

    if (maybeSave())
        loadFile(openFile(fileName));

}

void MainWindow::init_statusBar()
{

    lblMsg->setText("Ver");

    ui->statusbar->addWidget(ui->chkScope);
    ui->statusbar->addWidget(ui->chkDevice);
    ui->statusbar->addWidget(ui->chkMethod);
    ui->statusbar->addWidget(ui->chkName);
    ui->statusbar->addWidget(ui->editFind);
    ui->statusbar->addWidget(ui->btnFindPrevious);
    ui->statusbar->addWidget(ui->btnFindNext);
    ui->statusbar->addWidget(ui->editReplace);
    ui->statusbar->addWidget(ui->btnReplace);
    ui->statusbar->addWidget(ui->btnPreviousError);
    ui->statusbar->addWidget(ui->btnNextError);
    ui->statusbar->addWidget(ui->btnRefreshTree);

    ui->statusbar->addPermanentWidget(lblMsg);
}

void MainWindow::newFile()
{
    if(maybeSave())
    {

        textEdit->clear();
        ui->treeWidget->clear();
        ui->editShowMsg->clear();
        ui->editErrors->clear();
        ui->editWarnings->clear();
        ui->editRemarks->clear();
        setCurrentFile("");

    }
}



void MainWindow::on_btnReplaceFind_clicked()
{
    on_btnReplace_clicked();
    if(find_down)
        on_btnFindNext_clicked();
    if(find_up)
        on_btnFindPrevious_clicked();
}

void MainWindow::on_chkCaseSensitive_clicked()
{

}

void MainWindow::on_chkCaseSensitive_clicked(bool checked)
{
    CaseSensitive = checked;
}

/*菜单：设置字体*/
void MainWindow::set_font()
{

    bool ok;
    font = QFontDialog::getFont(&ok,this);

    if(ok)
    {

        textLexer->setFont(font);

        //存储字体信息
        QString qfile = QDir::homePath() + "/QtiASL.ini";
        QFile file(qfile);
        //QSettings Reg(qfile, QSettings::NativeFormat);
        QSettings Reg(qfile, QSettings::IniFormat);//全平台都采用ini格式
        Reg.setValue("FontName" , font.family());
        Reg.setValue("FontSize" , font.pointSize());
        Reg.setValue("FontBold" , font.bold());
        Reg.setValue("FontItalic" , font.italic());
        Reg.setValue("FontUnderline" , font.underline());

    }

}

/*菜单：是否自动换行*/
void MainWindow::set_wrap()
{
    if(ui->actionWrapWord->isChecked())
        textEdit->setWrapMode(QsciScintilla::WrapWord);
    else
        textEdit->setWrapMode(QsciScintilla::WrapNone);


}

void MainWindow::on_editFind_textChanged(const QString &arg1)
{
    if(arg1.count() > 0)
        on_btnFindNext_clicked();
    else
    {
        if(red < 55)
        {
            ui->editFind->setStyleSheet("background-color:rgba(50,50,50,255)");

        }
        else
        {
            ui->editFind->setStyleSheet("background-color:rgba(255,255,255,255)");

        }
    }
}

/*重载窗体重绘事件，用来刷新软件使用中，系统切换亮、暗模式*/
void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    //获取背景色
    QPalette pal = this->palette();
    QBrush brush = pal.window();
    int c_red = brush.color().red();
    if(c_red != red)
    {
        setLexer(textLexer);

        textEdit->repaint();
        //qDebug() << "setLexer";
    }


}


