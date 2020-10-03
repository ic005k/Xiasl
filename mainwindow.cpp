#include "mainwindow.h"
#include "ui_mainwindow.h"


bool loading = false;
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
QList<QTreeWidgetItem *> tw_list;
QTreeWidget *treeWidgetBak;

QString fileName;
QVector<QString> filelist;
QWidgetList wdlist;
QscilexerCppAttach *textLexer;

bool zh_cn = false;




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

    loadLocal();

    ver = "QtiASL V1.0.16    ";
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

    ui->actionKextstat->setEnabled(false);

#endif

#ifdef Q_OS_LINUX
// linux
    font.setFamily("SauceCodePro Nerd Font");
    font.setPointSize(13);

    ui->actionKextstat->setEnabled(false);
    ui->actionGenerate->setEnabled(false);
#endif

#ifdef Q_OS_MAC
// mac
    font.setFamily("SauceCodePro Nerd Font");
    font.setPointSize(13);

    ui->actionGenerate->setEnabled(false);

#endif

    int w = screen()->size().width();
    init_treeWidget(ui->treeWidget, w);
    treeWidgetBak = new QTreeWidget;
    init_treeWidget(treeWidgetBak, w);

    init_edit();

    init_info_edit();

    splitterMain = new QSplitter(Qt::Horizontal,this);
    splitterMain->addWidget(ui->tabWidget_misc);
    QSplitter *splitterRight = new QSplitter(Qt::Vertical,splitterMain);
    splitterRight->setOpaqueResize(true);
    splitterRight->addWidget(textEdit);
    splitterRight->addWidget(ui->tabWidget);
    ui->gridLayout->addWidget(splitterMain);
    ui->tabWidget->setHidden(true);

    ui->tabWidget_misc->setMaximumWidth(w/3 - 20);

    ui->chkName->setVisible(false);
    ui->chkScope->setVisible(false);
    ui->chkDevice->setVisible(false);
    ui->chkMethod->setVisible(false);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_linkage()));

    //最近打开的文件
    //Mac:"/Users/../Library/Preferences/com.github-com-ic005k-qtiasl.V1.plist"
    //Win:"\\HKEY_CURRENT_USER\\Software\\QtiASL\\V1"
    QCoreApplication::setOrganizationName("QtiASL");
    QCoreApplication::setOrganizationDomain("github.com/ic005k/QtiASL");
    QCoreApplication::setApplicationName("V1");

    m_recentFiles = new RecentFiles(this);
    if(!zh_cn)
       m_recentFiles->attachToMenuAfterItem(ui->menu_File, "SaveAS...", SLOT(recentOpen(QString)));//在此处插入菜单
    else
       m_recentFiles->attachToMenuAfterItem(ui->menu_File, "另存...", SLOT(recentOpen(QString)));//在此处插入菜单

    m_recentFiles->setNumOfRecentFiles(15);//最多显示最近的15个文件

    init_statusBar();

    init_filesystem();

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
    QString str;

    str = tr("Last modified: ");

    QString last = str + appInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
    QString str1 = "<a style='color:blue;' href = https://github.com/ic005k/QtiASL>QtiASL Editor</a><br><br>";

    QMessageBox::about(this , "About", str1 + last);

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

    }


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
    //关于是否采用GBK编码的方式，再考虑
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
    lblLayer->setText("");
    lblMsg->setText("");


    /*装入新文件，如果目前有线程在运行，则打断它*/
    if(!thread_end)
    {
        break_run = true; //通知打断线程

        mythread->quit();
        mythread->wait();
        /*延时1000ms，等待线程结束,以便刷新新文件的成员树*/
        QTime dieTime = QTime::currentTime().addMSecs(1000);
            while( QTime::currentTime() < dieTime )
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    }
    on_btnRefreshTree_clicked();

    loading = false;

}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->setModified(false);
    setWindowModified(false);

    shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.dsl";


    setWindowFilePath(shownName);

    setWindowTitle(ver + shownName);

    ui->btnPreviousError->setEnabled(false);
    ui->btnNextError->setEnabled(false);

    //初始化fsm
    QFileInfo f(shownName);
    ui->treeView->setRootIndex(model->index(f.path()));
    fsm_Index = model->index(f.path());
    ui->btnReturn->setText(shownName);

    ui->treeView->setCurrentIndex(model->index(shownName));//并设置当前条目为打开的文件
    ui->treeView->setFocus();

    QFileInfo fi(shownName);
    if(fi.suffix().toLower() == "dsl")
    {
        ui->actionWrapWord->setChecked(false);//取消自动换行，影响dsl文件开启速度
        textEdit->setWrapMode(QsciScintilla::WrapNone);

        //设置编译功能使能
        ui->actionCompiling->setEnabled(true);
        ui->btnCompile->setEnabled(true);

        ui->tabWidget_misc->setCurrentIndex(0);

    }
    else
    {
        ui->tabWidget_misc->setCurrentIndex(1);

        //设置编译功能屏蔽
        ui->actionCompiling->setEnabled(false);
        ui->btnCompile->setEnabled(false);

        ui->tabWidget->setVisible(false);
    }

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

    //QMessageBox::StandardButton ret;
    int ret;
    if(!zh_cn)
    {

            ret = QMessageBox::warning(this, tr("Application"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    }
    else
    {
            QMessageBox box(QMessageBox::Warning, "QtiASL","文件内容已修改，是否保存？");
            box.setStandardButtons (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            box.setButtonText (QMessageBox::Save,QString("保 存"));
            box.setButtonText (QMessageBox::Cancel,QString("取 消"));
            box.setButtonText (QMessageBox::Discard,QString("放 弃"));
            ret = box.exec ();

    }

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
    if(!zh_cn)
        statusBar()->showMessage(tr("File saved"), 2000);
    else
        statusBar()->showMessage("文件已保存", 2000);

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

    lblMsg->setText(tr("Compiling..."));

    qTime.start();

    QString op = ui->cboxCompilationOptions->currentText().trimmed();


#ifdef Q_OS_WIN32
// win
   co->start(appInfo.filePath() + "/iasl.exe" , QStringList() << op << curFile);
#endif

#ifdef Q_OS_LINUX
// linux
   co->start(appInfo.filePath() + "/iasl" , QStringList() << op << curFile);

#endif

#ifdef Q_OS_MAC
// mac
    co->start(appInfo.filePath() + "/iasl" , QStringList() << op << curFile);

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
    lblMsg->setText(tr("Compiled") + "(" + QTime::currentTime().toString() + "    " + QString::number(a, 'f', 2) + " s)");


    if(exitCode == 0)
    {

        ui->btnPreviousError->setEnabled(false);
        ui->btnNextError->setEnabled(false);
        ui->tabWidget->setCurrentIndex(0);

        if(!zh_cn)
            QMessageBox::information(this , "QtiASL" , "Compilation successful.");
        else
        {
            QMessageBox message(QMessageBox::Information, "QtiASL", tr("Compilation successful."));
            message.setStandardButtons (QMessageBox::Ok);
            message.setButtonText (QMessageBox::Ok, QString(tr("Ok")));
            message.exec();
        }

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

    ui->statusbar->showMessage(tr("Row") + " : " + QString::number(RowNum + 1) + "    " + tr("Column") + " : " + QString::number(ColNum));

    //联动treeWidget
    mem_linkage(ui->treeWidget);

}

/*换行之后，1s后再刷新成员树*/
void MainWindow::timer_linkage()
{
    if(!loading)
    {

         on_btnRefreshTree_clicked();

         timer->stop();

    }

}

/*单击文本任意位置，当前代码块与成员树进行联动*/
void MainWindow::mem_linkage(QTreeWidget * tw)
{

    int RowNum, ColNum;
    textEdit->getCursorPosition(&RowNum , &ColNum);
    /*进行联动的条件：装载文件没有进行&成员树不为空&不是始终在同一行里面*/
    if(!loading && tw->topLevelItemCount() > 0 && preRow != RowNum)
    {
        int treeSn = 0;
        QTreeWidgetItemIterator it(tw);
        textEditBack->setCursorPosition(RowNum, 0); //后台进行

        preRow = RowNum;

        for(int j = RowNum; j > -1; j--)//从当前行往上寻找Scope、Device、Method
        {
            QString str = textEditBack->text(j).trimmed();
            if(str.mid(0, 5) == "Scope" || str.mid(0, 5) == "Devic" || str.mid(0, 5) == "Metho")
            {

                while (*it)
                {
                    treeSn = (*it)->text(1).toInt();

                    if(treeSn == j)
                    {
                      tw->setCurrentItem((*it));
                      //状态栏上显示层次结构
                      lblLayer->setText(getLayerName((*it)));
                      //editLayer->setText(getLayerName((*it)));

                      break;

                    }

                    ++it;
                }

                break;
            }


        }

        //qDebug() << ColNum << RowNum;

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

            QPalette palette;
            palette.setColor(QPalette::Text,Qt::white);
            ui->editFind->setPalette(palette);

            palette = ui->editFind->palette();
            palette.setColor(QPalette::Base, QColor(50,50,50,255));
            ui->editFind->setPalette(palette);

        }
        else
        {

            QPalette palette;
            palette.setColor(QPalette::Text,Qt::black);
            ui->editFind->setPalette(palette);

            palette = ui->editFind->palette();
            palette.setColor(QPalette::Base, Qt::white);
            ui->editFind->setPalette(palette);

        }

    }
    else
    {
        if(str.count() > 0)
        {

            //字色
            QPalette palette;
            palette.setColor(QPalette::Text,Qt::white);
            ui->editFind->setPalette(palette);

            palette = ui->editFind->palette();
            palette.setColor(QPalette::Base, QColor(255,70,70));
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

    }
    else
    {
        if(red < 55)
        {


        }
        else
        {


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

                "External Scope Device Method Name If While Break Return ElseIf Switch Case Else "
                "Default Field OperationRegion Package DefinitionBlock Offset CreateDWordField CreateByteField "
                "CreateBitField CreateWordField CreateQWordField Buffer ToInteger ToString ToUUID ToUuid ToHexString ToDecimalString ToBuffer ToBcd"
                "CondRefOf FindSetLeftBit FindSetRightBit FromBcd Function CreateField "

                "Acquire Add Alias And "
                "BankField AccessAs CondRefOf ExtendedMemory ExtendedSpace "
                "BreakPoint Concatenate ConcatenateResTemplate Connection Continue CopyObject DataTableRegion Debug Decrement DerefOf "
                "Divide Dma Arg0 Arg1 Arg2 Arg3 Arg4 Arg5 Arg6 "
                "DWordIo DWordIO EisaId EndDependentFn Event ExtendedIo Fatal FixedDma FixedIo GpioInt GpioIo "
                "Increment Index IndexField Interrupt Io IO Irq IRQ IrqNoFlags "
                "LAnd LEqual LGreater LGreaterEqual LLess LLessEqual LNot LNotEqual Load LOr Match Mid Mod Multiply "
                "Mutex NAnd NoOp NOr Not Notify ObjectType Or PowerResource Revision "
                "Memory32Fixed "
                "DWordMemory Local0 Local1 Local2 Local3 Local4 Local5 Local6 Local7 "
                "DWordSpace One Ones Processor QWordIo QWordIO Memory24 Memory32 VendorLong VendorShort Wait WordBusNumber WordIo WordSpace "
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

        timer->start(1000);

}

void thread_one::run()
{


    if(break_run)
    {

        return;
    }

    thread_end = false;

    //refreshTree();//之前预留，准备弃用
    getMemberTree(textEditBack);

    //emit over();

    QMetaObject::invokeMethod(this, "over");


}

/*线程结束后对成员树进行数据刷新*/
void MainWindow::dealover()
{

    //update_ui_tw();//之前预留，准备弃用
    update_ui_tree();

    thread_end = true;
    break_run = false;


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

void MainWindow::update_ui_tree()
{

    if(break_run){return;}

    ui->treeWidget->clear();
    ui->treeWidget->update();
    ui->treeWidget->addTopLevelItems(tw_list);
    ui->treeWidget->expandAll();


    ui->treeWidget->setHeaderLabel("Scope(" + QString::number(s_count) + ")  " + "Device(" + QString::number(d_count) + ")  " + "Method(" + QString::number(m_count) + ")");//  + "N(" + QString::number(n_count) + ")");
    ui->treeWidget->update();

    float a = qTime.elapsed()/1000.00;
    lblMsg->setText(tr("Refresh completed") + "(" + QTime::currentTime().toString() + "    " + QString::number(a, 'f', 2) + " s)");

    textEdit_cursorPositionChanged();

    QFileInfo fi(curFile);
    if(fi.suffix().toLower() == "dsl")
    {
        ui->treeWidget->setHidden(false);
    }

}

void MainWindow::update_ui_tw()
{
    ui->treeWidget->clear();

    ui->treeWidget->update();

    ui->treeWidget->addTopLevelItems(twitems);

    ui->treeWidget->sortItems(1 , Qt::AscendingOrder);//排序

    ui->treeWidget->setIconSize(QSize(12, 12));

    ui->treeWidget->setHeaderLabel("S(" + QString::number(s_count) + ")  " + "D(" + QString::number(d_count) + ")  " + "M(" + QString::number(m_count) + ")  "  + "N(" + QString::number(n_count) + ")");
    ui->treeWidget->update();

    float a = qTime.elapsed()/1000.00;
    lblMsg->setText("Refresh completed(" + QTime::currentTime().toString() + "    " + QString::number(a, 'f', 2) + " s)");

    textEdit_cursorPositionChanged();

    QFileInfo fi(curFile);
    if(fi.suffix().toLower() == "dsl")
    {
        ui->treeWidget->setHidden(false);
    }

}

void MainWindow::on_btnRefreshTree_clicked()
{


    if(!thread_end)
    {
        break_run = true;
        //lblMsg->setText("Refresh interrupted");
        mythread->quit();
        mythread->wait();

        /*等待线程结束,以使最后一次刷新可以完成*/
        while(!thread_end)
        {
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

QString getMemberName(QString str_member, QsciScintilla *textEdit, int RowNum)
{
    //int RowNum, ColNum;
    QString sub;
    //textEdit->getCursorPosition(&RowNum, &ColNum);

    sub = textEdit->text(RowNum).trimmed();

    QString str_end;
    if(sub.mid(0 , str_member.count()) == str_member)
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

void MainWindow::set_mark(int linenr)
{
    //SCI_MARKERGET 参数用来设置标记，默认为圆形标记
    textEdit->SendScintilla(QsciScintilla::SCI_MARKERGET, linenr);
    //SCI_MARKERSETFORE，SCI_MARKERSETBACK设置标记前景和背景标记
    textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0, QColor(Qt::red));
    textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0, QColor(Qt::red));
    textEdit->SendScintilla(QsciScintilla::SCI_MARKERADD, linenr);

}

int getBraceScope(int start, int count, QsciScintilla *textEdit)
{
    int dkh1 = 0;
    int scope_end = 0;
    bool end = false;
    /*start-1,从当前行就开始解析，囊括Scope(){等这种紧跟{的写法*/
    for(int s = start - 1; s < count; s++)
    {

        QString str = textEdit->text(s);

        for(int t = 0; t < str.count(); t++)
        {
            if(str.mid(t, 1) == "{")
            {
                dkh1 ++;

            }
            if(str.mid(t, 1) == "}")
            {
                dkh1 --;

                if(dkh1 == 0)
                {
                    //范围结束
                    int row, col;
                    textEdit->getCursorPosition(&row, &col);
                    scope_end = s + 1;
                    end = true;
                    //qDebug() << "范围结束" << scope_end;
                    break;

                }
            }

        }

        if(end)
        {

            break;
        }

    }

    /*如果没有找到匹配的}，则返回开始位置的下一行，否则会进行无限循环*/
    if(!end)
        return start + 1;

    return scope_end;
}

bool chkMemberName(QString str, QString name)
{


   if(str.trimmed().mid(0, name.count()) == name)
       return true;


    return false;

}

void addSubItem(int start, int end, QsciScintilla *textEdit, QString Name, QTreeWidgetItem *iTop)
{

    textEdit->setCursorPosition(start, 0);

    for(int sdds1 = start; sdds1 < end; sdds1++)
    {
       if(break_run)
           break;

       QString str = textEdit->text(sdds1).trimmed();

       if(chkMemberName(str, Name))
        {

              QTreeWidgetItem *iSub = new QTreeWidgetItem(QStringList() << getMemberName(Name, textEdit, sdds1) << QString("%1").arg(sdds1, 7, 10, QChar('0')));

              if(Name == "Device")
              {
                iSub->setIcon(0, QIcon(":/icon/d.png"));
                d_count++;
              }
              if(Name == "Scope")
              {
                iSub->setIcon(0, QIcon(":/icon/s.png"));
                s_count++;
              }
              if(Name == "Method")
              {
                iSub->setIcon(0, QIcon(":/icon/m.png"));
                m_count++;
              }


              iTop->addChild(iSub);


        }

   }

}

QTreeWidgetItem *addChildItem(int row, QsciScintilla *textEdit, QString Name, QTreeWidgetItem *iTop)
{
    QTreeWidgetItem *iSub = new QTreeWidgetItem(QStringList() << getMemberName(Name, textEdit, row) << QString("%1").arg(row, 7, 10, QChar('0')));
    if(Name == "Device")
    {
      iSub->setIcon(0, QIcon(":/icon/d.png"));
      d_count++;
    }
    if(Name == "Scope")
    {
      iSub->setIcon(0, QIcon(":/icon/s.png"));
      s_count++;
    }
    if(Name == "Method")
    {
      iSub->setIcon(0, QIcon(":/icon/m.png"));
      m_count++;
    }

    iTop->addChild(iSub);

    return iSub;


}

void getMemberTree(QsciScintilla *textEdit)
{

    if(break_run){return;}

    loading = true;

    tw_list.clear();

    s_count = 0;
    m_count = 0;
    d_count = 0;
    n_count = 0;

    QString str_member;

    int count;  //总行数

    QTreeWidgetItem *twItem0;
    count = textEdit->lines();

    for(int j = 0; j < count; j++)
    {
        if(break_run)
            break;

        str_member = textEdit->text(j).trimmed();

        //根"Scope"
        if(chkMemberName(str_member, "Scope"))
        {


           twItem0 = new QTreeWidgetItem(QStringList() << getMemberName(str_member, textEdit, j) << QString("%1").arg(j, 7, 10, QChar('0')));
           twItem0->setIcon(0, QIcon(":/icon/s.png"));
           //tw->addTopLevelItem(twItem0);
           tw_list.append(twItem0);

           s_count++;


           int c_fw_start = j + 1;
           int c_fw_end = getBraceScope(c_fw_start, count, textEdit);

           //再往下找内部成员

           for(int d = c_fw_start; d < c_fw_end; d++)
           {

               if(break_run)
                   break;

               QString str = textEdit->text(d).trimmed();

               //Scope-->Device
               if(chkMemberName(str, "Device"))
                {

                       QTreeWidgetItem *twItem1 = addChildItem(d, textEdit, "Device", twItem0);

                       int d2_start = d + 1;
                       int d2_end = getBraceScope(d2_start, count, textEdit);

                       for(int m2 = d2_start; m2 < d2_end; m2++)
                       {
                           if(break_run)
                               break;

                           QString str = textEdit->text(m2).trimmed();
                            //Scope-->Device-->Method
                            if(chkMemberName(str, "Method"))
                             {

                                QTreeWidgetItem *twItem2 = addChildItem(m2, textEdit, "Method", twItem1);
                                if(twItem2){}

                             }

                             //Scope-->Device-->Device
                            if(chkMemberName(str, "Device"))
                             {


                                QTreeWidgetItem *twItem2 = addChildItem(m2, textEdit, "Device", twItem1);

                                int start = m2 + 1;
                                int end = getBraceScope(start, count, textEdit);

                                for(int sddm1 = start; sddm1 < end; sddm1++)
                                 {
                                    if(break_run)
                                        break;

                                    QString str = textEdit->text(sddm1).trimmed();

                                    //Scope-->Device-->Device-->Method
                                    if(chkMemberName(str, "Method"))
                                     {

                                           QTreeWidgetItem *twItem3 = addChildItem(sddm1, textEdit, "Method", twItem2);
                                           if(twItem3){}

                                     }

                                    //Scope-->Device-->Device-->Scope
                                    if(chkMemberName(str, "Scope"))
                                     {

                                           QTreeWidgetItem *twItem3 = addChildItem(sddm1, textEdit, "Scope", twItem2);

                                           int start_sdds = sddm1 + 1;
                                           int end_sdds = getBraceScope(start_sdds, count, textEdit);

                                           for(int sdds = start_sdds; sdds < end_sdds; sdds++)
                                            {
                                               if(break_run){break;}

                                               QString str = textEdit->text(sdds).trimmed();

                                               //S--D--D--S--S
                                               if(chkMemberName(str, "Scope"))
                                                {

                                                   QTreeWidgetItem *twItem4 = addChildItem(sdds, textEdit, "Scope", twItem3);
                                                   if(twItem4){}

                                                   int start_sddss = sdds + 1;
                                                   int end_sddss = getBraceScope(start_sddss, count, textEdit);
                                                   for(int sddss = start_sddss; sddss < end_sddss; sddss ++){
                                                       if(break_run){break;}
                                                        QString str = textEdit->text(sddss);

                                                        //S--D--D--S--S--S
                                                        if(chkMemberName(str, "Scope")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddss, textEdit, "Scope", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--S--S--D
                                                        if(chkMemberName(str, "Device")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddss, textEdit, "Device", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--S--S--M
                                                        if(chkMemberName(str, "Method")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddss, textEdit, "Method", twItem4);
                                                            if(twItem5){}
                                                        }
                                                   }

                                                   sdds = end_sddss - 1;

                                                }

                                               //S--D--D--S--M
                                               if(chkMemberName(str, "Method"))
                                                {

                                                   QTreeWidgetItem *twItem4 = addChildItem(sdds, textEdit, "Method", twItem3);
                                                   if(twItem4){}

                                                }

                                               //Scope-->Device-->Device-->Scope-->Device
                                               if(chkMemberName(str, "Device"))
                                                {

                                                   QTreeWidgetItem *twItem4 = addChildItem(sdds, textEdit, "Device", twItem3);
                                                   if(twItem4){}

                                                   int start_sddsd = sdds + 1;
                                                   int end_sddsd = getBraceScope(start_sddsd, count, textEdit);
                                                   for(int sddsd = start_sddsd; sddsd < end_sddsd; sddsd ++){
                                                       if(break_run){break;}
                                                        QString str = textEdit->text(sddsd);

                                                        //S--D--D--S--D--S
                                                        if(chkMemberName(str, "Scope")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddsd, textEdit, "Scope", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--S--D--D
                                                        if(chkMemberName(str, "Device")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddsd, textEdit, "Device", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--S--D--M
                                                        if(chkMemberName(str, "Method")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddsd, textEdit, "Method", twItem4);
                                                            if(twItem5){}
                                                        }
                                                   }

                                                   sdds = end_sddsd - 1;

                                                }

                                           }

                                        sddm1 = end_sdds - 1;

                                     }



                                    //Scope-->Device-->Device-->Device
                                    if(chkMemberName(str, "Device"))
                                     {

                                           QTreeWidgetItem *twItem3 = addChildItem(sddm1, textEdit, "Device", twItem2);

                                           int start3 = sddm1 + 1;
                                           int end3 = getBraceScope(start3, count, textEdit);

                                           for(int sddd = start3; sddd < end3; sddd ++)
                                           {
                                               QString str = textEdit->text(sddd).trimmed();

                                               //Scope-->Device-->Device-->Device-->Method
                                               if(chkMemberName(str, "Method"))
                                                {

                                                   QTreeWidgetItem *twItem4 = addChildItem(sddd, textEdit, "Method", twItem3);
                                                   if(twItem4){}

                                                }

                                               //Scope-->Device-->Device-->Device-->Scope
                                               if(chkMemberName(str, "Scope"))
                                                {

                                                   QTreeWidgetItem *twItem4 = addChildItem(sddd, textEdit, "Scope", twItem3);
                                                   if(twItem4){}

                                                   int start_sddds = sddd + 1;
                                                   int end_sddds = getBraceScope(start_sddds, count, textEdit);
                                                   for(int sddds = start_sddds; sddds < end_sddds; sddds ++){
                                                       if(break_run){break;}
                                                        QString str = textEdit->text(sddds);

                                                        //S--D--D--D--S--S
                                                        if(chkMemberName(str, "Scope")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddds, textEdit, "Scope", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--D--S--D
                                                        if(chkMemberName(str, "Device")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddds, textEdit, "Device", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--D--S--M
                                                        if(chkMemberName(str, "Method")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sddds, textEdit, "Method", twItem4);
                                                            if(twItem5){}
                                                        }
                                                   }

                                                   sddd = end_sddds - 1;

                                                }

                                               //Scope-->Device-->Device-->Device-->Device
                                               if(chkMemberName(str, "Device"))
                                                {
                                                   QTreeWidgetItem *twItem4 = addChildItem(sddd, textEdit, "Device", twItem3);
                                                   if(twItem4){}

                                                   int start_sdddd = sddd + 1;
                                                   int end_sdddd = getBraceScope(start_sdddd, count, textEdit);
                                                   for(int sdddd = start_sdddd; sdddd < end_sdddd; sdddd ++){
                                                       if(break_run){break;}
                                                        QString str = textEdit->text(sdddd);

                                                        //S--D--D--D--D--S
                                                        if(chkMemberName(str, "Scope")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdddd, textEdit, "Scope", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--D--D--D
                                                        if(chkMemberName(str, "Device")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdddd, textEdit, "Device", twItem4);
                                                            if(twItem5){}
                                                        }

                                                        //S--D--D--D--D--M
                                                        if(chkMemberName(str, "Method")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdddd, textEdit, "Method", twItem4);
                                                            if(twItem5){}
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

                            //Scope-->Device-->Scope
                            if(chkMemberName(str, "Scope"))
                             {
                                QTreeWidgetItem *twItem2 = addChildItem(m2, textEdit, "Scope", twItem1);
                                if(twItem2){}


                                int start_sds = m2 + 1;
                                int end_sds = getBraceScope(start_sds, count, textEdit);

                                for(int sds = start_sds; sds < end_sds; sds++)
                                {
                                       if(break_run)
                                           break;

                                       QString str = textEdit->text(sds).trimmed();

                                       //Scope-->Device-->Scope-->Scope
                                       if(chkMemberName(str, "Scope"))
                                       {

                                           QTreeWidgetItem *twItem3 = addChildItem(sds, textEdit, "Scope", twItem2);
                                           if(twItem3){}

                                           int start_sdss = sds + 1;
                                           int end_sdss = getBraceScope(start_sdss, count, textEdit);
                                           for(int sdss = start_sdss; sdss < end_sdss; sdss ++){
                                               if(break_run){break;}
                                               QString str = textEdit->text(sdss);

                                               //S--D--S--S--S
                                               if(chkMemberName(str, "Scope")){
                                                    QTreeWidgetItem *twItem4 = addChildItem(sdss, textEdit, "Scope", twItem3);
                                                    if(twItem4){}

                                                    int start_sdsss = sdss + 1;
                                                    int end_sdsss = getBraceScope(start_sdsss, count, textEdit);
                                                    for(int sdsss = start_sdsss; sdsss < end_sdsss; sdsss ++){
                                                        if(break_run){break;}
                                                        QString str = textEdit->text(sdsss);

                                                        //S--D--S--S--S--S
                                                        if(chkMemberName(str, "Scope")){
                                                             QTreeWidgetItem *twItem5 = addChildItem(sdsss, textEdit, "Scope", twItem4);
                                                             if(twItem5){}

                                                             int start_sdssss = sdsss + 1;
                                                             int end_sdssss = getBraceScope(start_sdssss, count, textEdit);
                                                             for(int sdssss = start_sdssss; sdssss < end_sdssss; sdssss ++){
                                                                 if(break_run){break;}
                                                                 QString str = textEdit->text(sdssss);

                                                                 //S--D--S--S--S--S--S
                                                                 if(chkMemberName(str, "Scope")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssss, textEdit, "Scope", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--S--S--D
                                                                 if(chkMemberName(str, "Device")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssss, textEdit, "Device", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--S--S--M
                                                                 if(chkMemberName(str, "Method")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssss, textEdit, "Method", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                             }

                                                          sdsss = end_sdssss - 1;
                                                         }
                                                        //S--D--S--S--S--D
                                                        if(chkMemberName(str, "Device")){
                                                             QTreeWidgetItem *twItem5 = addChildItem(sdsss, textEdit, "Device", twItem4);
                                                             if(twItem5){}

                                                             int start_sdsssd = sdsss + 1;
                                                             int end_sdsssd = getBraceScope(start_sdsssd, count, textEdit);
                                                             for(int sdsssd = start_sdsssd; sdsssd < end_sdsssd; sdsssd ++){
                                                                 if(break_run){break;}
                                                                 QString str = textEdit->text(sdsssd);

                                                                 //S--D--S--S--S--D--S
                                                                 if(chkMemberName(str, "Scope")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdsssd, textEdit, "Scope", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--S--D--D
                                                                 if(chkMemberName(str, "Device")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdsssd, textEdit, "Device", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--S--D--M
                                                                 if(chkMemberName(str, "Method")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdsssd, textEdit, "Method", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                             }

                                                          sdsss = end_sdsssd - 1;
                                                         }
                                                        //S--D--S--S--S--M
                                                        if(chkMemberName(str, "Method")){
                                                             QTreeWidgetItem *twItem5 = addChildItem(sdsss, textEdit, "Method", twItem4);
                                                             if(twItem5){}
                                                         }

                                                    }
                                                    sdss = end_sdsss - 1;
                                                }

                                               //S--D--S--S--D
                                               if(chkMemberName(str, "Device")){
                                                    QTreeWidgetItem *twItem4 = addChildItem(sdss, textEdit, "Device", twItem3);
                                                    if(twItem4){}

                                                    int start_sdssd = sdss + 1;
                                                    int end_sdssd = getBraceScope(start_sdssd, count, textEdit);
                                                    for(int sdssd = start_sdssd; sdssd < end_sdssd; sdssd ++){
                                                        if(break_run){break;}
                                                        QString str = textEdit->text(sdssd);

                                                        //S--D--S--S--D--S
                                                        if(chkMemberName(str, "Scope")){
                                                             QTreeWidgetItem *twItem5 = addChildItem(sdssd, textEdit, "Scope", twItem4);
                                                             if(twItem5){}

                                                             int start_sdssds = sdssd + 1;
                                                             int end_sdssds = getBraceScope(start_sdssds, count, textEdit);
                                                             for(int sdssds = start_sdssds; sdssds < end_sdssds; sdssds ++){
                                                                 if(break_run){break;}
                                                                 QString str = textEdit->text(sdssds);

                                                                 //S--D--S--S--D--S--S
                                                                 if(chkMemberName(str, "Scope")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssds, textEdit, "Scope", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--D--S--D
                                                                 if(chkMemberName(str, "Device")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssds, textEdit, "Device", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--D--S--M
                                                                 if(chkMemberName(str, "Method")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssds, textEdit, "Method", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                             }

                                                          sdssd = end_sdssds - 1;
                                                         }
                                                        //S--D--S--S--D--D
                                                        if(chkMemberName(str, "Device")){
                                                             QTreeWidgetItem *twItem5 = addChildItem(sdssd, textEdit, "Device", twItem4);
                                                             if(twItem5){}

                                                             int start_sdssdd = sdssd + 1;
                                                             int end_sdssdd = getBraceScope(start_sdssdd, count, textEdit);
                                                             for(int sdssdd = start_sdssdd; sdssdd < end_sdssdd; sdssdd ++){
                                                                 if(break_run){break;}
                                                                 QString str = textEdit->text(sdssdd);

                                                                 //S--D--S--S--D--D--S
                                                                 if(chkMemberName(str, "Scope")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssdd, textEdit, "Scope", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--D--D--D
                                                                 if(chkMemberName(str, "Device")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssdd, textEdit, "Device", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                                 //S--D--S--S--D--D--M
                                                                 if(chkMemberName(str, "Method")){
                                                                      QTreeWidgetItem *twItem6 = addChildItem(sdssdd, textEdit, "Method", twItem5);
                                                                      if(twItem6){}
                                                                  }
                                                             }

                                                          sdssd = end_sdssdd - 1;
                                                         }
                                                        //S--D--S--S--D--M
                                                        if(chkMemberName(str, "Method")){
                                                             QTreeWidgetItem *twItem5 = addChildItem(sdssd, textEdit, "Method", twItem4);
                                                             if(twItem5){}
                                                         }

                                                    }

                                                    sdss = end_sdssd - 1;

                                                }

                                               //S--D--S--S--M
                                               if(chkMemberName(str, "Method")){
                                                    QTreeWidgetItem *twItem4 = addChildItem(sdss, textEdit, "Method", twItem3);
                                                    if(twItem4){}
                                                }
                                           }

                                           sds = end_sdss - 1;

                                       }


                                       //Scope-->Device-->Scope-->Device
                                       if(chkMemberName(str, "Device"))
                                        {
                                           QTreeWidgetItem *twItem3 = addChildItem(sds, textEdit, "Device", twItem2);
                                           if(twItem3){}

                                           int start4 = sds + 1;
                                           int end4 = getBraceScope(start4, count, textEdit);

                                           for(int m4 = start4; m4 < end4; m4++)
                                            {

                                               if(break_run)
                                                   break;

                                               QString str = textEdit->text(m4).trimmed();

                                               //Scope-->Device-->Scope-->Device-->Method
                                               if(chkMemberName(str, "Method"))
                                                {
                                                   QTreeWidgetItem *twItem4 = addChildItem(m4, textEdit, "Method", twItem3);
                                                   if(twItem4){}


                                                }

                                               //Scope-->Device-->Scope-->Device-->Device
                                               if(chkMemberName(str, "Device"))
                                                {
                                                   QTreeWidgetItem *twItem4 = addChildItem(m4, textEdit, "Device", twItem3);
                                                   if(twItem4){}

                                                   int start_sdsdd = m4 + 1;
                                                   int end_sdsdd = getBraceScope(start_sdsdd, count, textEdit);
                                                   for(int sdsdd = start_sdsdd; sdsdd < end_sdsdd; sdsdd ++){
                                                       if(break_run){break;}
                                                       QString str = textEdit->text(sdsdd);

                                                       //S--D--S--D--D--S
                                                       if(chkMemberName(str, "Scope")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdsdd, textEdit, "Scope", twItem4);
                                                            if(twItem5){}
                                                        }
                                                       //S--D--S--D--D--D
                                                       if(chkMemberName(str, "Device")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdsdd, textEdit, "Device", twItem4);
                                                            if(twItem5){}
                                                        }
                                                       //S--D--S--D--D--M
                                                       if(chkMemberName(str, "Method")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdsdd, textEdit, "Method", twItem4);
                                                            if(twItem5){}
                                                        }
                                                   }

                                                m4 = end_sdsdd - 1;

                                                }

                                               //Scope-->Device-->Scope-->Device-->Scope
                                               if(chkMemberName(str, "Scope"))
                                                {
                                                   QTreeWidgetItem *twItem4 = addChildItem(m4, textEdit, "Scope", twItem3);
                                                   if(twItem4){}

                                                   int start_sdsds = m4 + 1;
                                                   int end_sdsds = getBraceScope(start_sdsds, count, textEdit);
                                                   for(int sdsds = start_sdsds; sdsds < end_sdsds; sdsds ++){
                                                       if(break_run){break;}
                                                       QString str = textEdit->text(sdsds);

                                                       //S--D--S--D--S--S
                                                       if(chkMemberName(str, "Scope")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdsds, textEdit, "Scope", twItem4);
                                                            if(twItem5){}

                                                            int start_sdsdss = sdsds + 1;
                                                            int end_sdsdss = getBraceScope(start_sdsdss, count, textEdit);
                                                            for(int sdsdss = start_sdsdss; sdsdss < end_sdsdss; sdsdss ++){
                                                                if(break_run){break;}
                                                                QString str = textEdit->text(sdsdss);

                                                                //S--D--S--D--S--S--S
                                                                if(chkMemberName(str, "Scope")){
                                                                     QTreeWidgetItem *twItem6 = addChildItem(sdsdss, textEdit, "Scope", twItem5);
                                                                     if(twItem6){}
                                                                 }
                                                                //S--D--S--D--S--S--D
                                                                if(chkMemberName(str, "Device")){
                                                                     QTreeWidgetItem *twItem6 = addChildItem(sdsdss, textEdit, "Device", twItem5);
                                                                     if(twItem6){}
                                                                 }
                                                                //S--D--S--D--S--S--M
                                                                if(chkMemberName(str, "Method")){
                                                                     QTreeWidgetItem *twItem6 = addChildItem(sdsdss, textEdit, "Method", twItem5);
                                                                     if(twItem6){}
                                                                 }
                                                            }

                                                         sdsds = end_sdsdss - 1;
                                                        }
                                                       //S--D--S--D--S--D
                                                       if(chkMemberName(str, "Device")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdsds, textEdit, "Device", twItem4);
                                                            if(twItem5){}

                                                            int start_sdsdsd = sdsds + 1;
                                                            int end_sdsdsd = getBraceScope(start_sdsdsd, count, textEdit);
                                                            for(int sdsdsd = start_sdsdsd; sdsdsd < end_sdsdsd; sdsdsd ++){
                                                                if(break_run){break;}
                                                                QString str = textEdit->text(sdsdsd);

                                                                //S--D--S--D--S--D--S
                                                                if(chkMemberName(str, "Scope")){
                                                                     QTreeWidgetItem *twItem6 = addChildItem(sdsdsd, textEdit, "Scope", twItem5);
                                                                     if(twItem6){}
                                                                 }
                                                                //S--D--S--D--S--D--D
                                                                if(chkMemberName(str, "Device")){
                                                                     QTreeWidgetItem *twItem6 = addChildItem(sdsdsd, textEdit, "Device", twItem5);
                                                                     if(twItem6){}
                                                                 }
                                                                //S--D--S--D--S--D--M
                                                                if(chkMemberName(str, "Method")){
                                                                     QTreeWidgetItem *twItem6 = addChildItem(sdsdsd, textEdit, "Method", twItem5);
                                                                     if(twItem6){}
                                                                 }
                                                            }

                                                         sdsds = end_sdsdsd - 1;
                                                        }
                                                       //S--D--S--D--S--M
                                                       if(chkMemberName(str, "Method")){
                                                            QTreeWidgetItem *twItem5 = addChildItem(sdsds, textEdit, "Method", twItem4);
                                                            if(twItem5){}
                                                        }
                                                   }

                                                m4 = end_sdsds - 1;

                                                }



                                             }

                                              sds = end4 - 1;
                                         }

                                        //Scope-->Device-->Scope-->Method
                                        if(chkMemberName(str, "Method"))
                                        {

                                            QTreeWidgetItem *twItem3 = addChildItem(sds, textEdit, "Method", twItem2);
                                            if(twItem3){}

                                         }



                                }

                                m2 = end_sds - 1;

                              }


                       }

                       d = d2_end - 1;

                   }

                   //S--S
                   if(chkMemberName(str, "Scope"))
                   {
                       QTreeWidgetItem *twItem1 = addChildItem(d, textEdit, "Scope", twItem0);
                       if(twItem1){}

                       int start_ss = d + 1;
                       int end_ss = getBraceScope(start_ss, count, textEdit);
                       for(int ss = start_ss; ss < end_ss; ss ++){
                           if(break_run){break;}
                           QString str = textEdit->text(ss);

                           //S--S--S
                           if(chkMemberName(str, "Scope")){
                                QTreeWidgetItem *twItem2 = addChildItem(ss, textEdit, "Scope", twItem1);
                                if(twItem2){}

                                int start_sss = ss + 1;
                                int end_sss = getBraceScope(start_sss, count, textEdit);
                                for(int sss = start_sss; sss < end_sss; sss ++){
                                    if(break_run){break;}
                                    QString str = textEdit->text(sss);

                                    //S--S--S--S
                                    if(chkMemberName(str, "Scope")){
                                         QTreeWidgetItem *twItem3 = addChildItem(sss, textEdit, "Scope", twItem2);
                                         if(twItem3){}
                                     }
                                    //S--S--S--D
                                    if(chkMemberName(str, "Device")){
                                         QTreeWidgetItem *twItem3 = addChildItem(sss, textEdit, "Device", twItem2);
                                         if(twItem3){}
                                     }
                                    //S--S--S--M
                                    if(chkMemberName(str, "Method")){
                                         QTreeWidgetItem *twItem3 = addChildItem(sss, textEdit, "Method", twItem2);
                                         if(twItem3){}
                                     }
                                }

                             ss = end_sss - 1;
                            }
                           //S--S--D
                           if(chkMemberName(str, "Device")){
                                QTreeWidgetItem *twItem2 = addChildItem(ss, textEdit, "Device", twItem1);
                                if(twItem2){}

                                int start_ssd = ss + 1;
                                int end_ssd = getBraceScope(start_ssd, count, textEdit);
                                for(int ssd = start_ssd; ssd < end_ssd; ssd ++){
                                    if(break_run){break;}
                                    QString str = textEdit->text(ssd);

                                    //S--S--D--S
                                    if(chkMemberName(str, "Scope")){
                                         QTreeWidgetItem *twItem3 = addChildItem(ssd, textEdit, "Scope", twItem2);
                                         if(twItem3){}
                                     }
                                    //S--S--D--D
                                    if(chkMemberName(str, "Device")){
                                         QTreeWidgetItem *twItem3 = addChildItem(ssd, textEdit, "Device", twItem2);
                                         if(twItem3){}
                                     }
                                    //S--S--D--M
                                    if(chkMemberName(str, "Method")){
                                         QTreeWidgetItem *twItem3 = addChildItem(ssd, textEdit, "Method", twItem2);
                                         if(twItem3){}
                                     }
                                }

                             ss = end_ssd - 1;
                            }
                           //S--S--M
                           if(chkMemberName(str, "Method")){
                                QTreeWidgetItem *twItem2 = addChildItem(ss, textEdit, "Method", twItem1);
                                if(twItem2){}
                            }
                       }

                    d = end_ss - 1;
                   }

                   //S--M
                   if(chkMemberName(str, "Method"))
                   {

                       QTreeWidgetItem *twItem1 = addChildItem(d, textEdit, "Method", twItem0);
                       if(twItem1){}

                   }
            }

           j = c_fw_end - 1;


         }

        //根下的"Method"
        if(chkMemberName(str_member, "Method"))
        {

           QTreeWidgetItem *twItem0 = new QTreeWidgetItem(QStringList() << getMemberName(str_member, textEdit, j) << QString("%1").arg(j, 7, 10, QChar('0')));
           twItem0->setIcon(0, QIcon(":/icon/m.png"));
           //tw->addTopLevelItem(twItem0);
           tw_list.append(twItem0);

           m_count++;


        }

        //根下的"Device"
        if(chkMemberName(str_member, "Device"))
        {

           QTreeWidgetItem *twItem0 = new QTreeWidgetItem(QStringList() << getMemberName(str_member, textEdit, j) << QString("%1").arg(j, 7, 10, QChar('0')));
           twItem0->setIcon(0, QIcon(":/icon/d.png"));
           //tw->addTopLevelItem(twItem0);
           tw_list.append(twItem0);

           d_count++;

           int start_d = j + 1;
           int end_d = getBraceScope(start_d, count, textEdit);

           for(int d = start_d; d < end_d; d++)
           {
               if(break_run)
                   break;

               QString str = textEdit->text(d);

               //D--S
               if(chkMemberName(str, "Scope"))
               {
                   QTreeWidgetItem *twItem1 = addChildItem(d, textEdit, "Scope", twItem0);
                   if(twItem1){}

                   int start_ds = d + 1;
                   int end_ds = getBraceScope(start_ds, count, textEdit);
                   for(int ds = start_ds; ds < end_ds; ds ++){
                       if(break_run){break;}
                       QString str = textEdit->text(ds);

                       //D--S--S
                       if(chkMemberName(str, "Scope")){
                            QTreeWidgetItem *twItem2 = addChildItem(ds, textEdit, "Scope", twItem1);
                            if(twItem2){}

                            int start_dss = ds + 1;
                            int end_dss = getBraceScope(start_dss, count, textEdit);
                            for(int dss = start_dss; dss < end_dss; dss ++){
                                if(break_run){break;}
                                QString str = textEdit->text(dss);

                                //D--S--S--S
                                if(chkMemberName(str, "Scope")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dss, textEdit, "Scope", twItem2);
                                     if(twItem3){}

                                     int start_dsss = dss + 1;
                                     int end_dsss = getBraceScope(start_dsss, count, textEdit);
                                     for(int dsss = start_dsss; dsss < end_dsss; dsss ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(dsss);

                                         //D--S--S--S--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsss, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--S--S--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsss, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--S--S--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsss, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  dss = end_dsss - 1;
                                 }
                                //D--S--S--D
                                if(chkMemberName(str, "Device")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dss, textEdit, "Device", twItem2);
                                     if(twItem3){}

                                     int start_dssd = dss + 1;
                                     int end_dssd = getBraceScope(start_dssd, count, textEdit);
                                     for(int dssd = start_dssd; dssd < end_dssd; dssd ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(dssd);

                                         //D--S--S--D--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dssd, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--S--D--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dssd, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--S--D--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dssd, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  dss = end_dssd - 1;
                                 }
                                //D--S--S--M
                                if(chkMemberName(str, "Method")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dss, textEdit, "Method", twItem2);
                                     if(twItem3){}
                                 }
                            }

                         ds = end_dss - 1;
                        }
                       //D--S--D
                       if(chkMemberName(str, "Device")){
                            QTreeWidgetItem *twItem2 = addChildItem(ds, textEdit, "Device", twItem1);
                            if(twItem2){}

                            int start_dsd = ds + 1;
                            int end_dsd = getBraceScope(start_dsd, count, textEdit);
                            for(int dsd = start_dsd; dsd < end_dsd; dsd ++){
                                if(break_run){break;}
                                QString str = textEdit->text(dsd);

                                //D--S--D--S
                                if(chkMemberName(str, "Scope")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dsd, textEdit, "Scope", twItem2);
                                     if(twItem3){}

                                     /*下一个子层*/
                                     int start_dsds = dsd + 1;
                                     int end_dsds = getBraceScope(start_dsds, count, textEdit);
                                     for(int dsds = start_dsds; dsds < end_dsds; dsds ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(dsds);

                                         //D--S--D--S--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsds, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--D--S--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsds, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--D--S--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsds, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  dsd = end_dsds - 1;
                                 }
                                //D--S--D--D
                                if(chkMemberName(str, "Device")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dsd, textEdit, "Device", twItem2);
                                     if(twItem3){}

                                     /*下一个子层*/
                                     int start_dsdd = dsd + 1;
                                     int end_dsdd = getBraceScope(start_dsdd, count, textEdit);
                                     for(int dsdd = start_dsdd; dsdd < end_dsdd; dsdd ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(dsdd);

                                         //D--S--D--D--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsdd, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--D--D--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsdd, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--S--D--D--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dsdd, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  dsd = end_dsdd - 1;
                                 }
                                //D--S--D--M
                                if(chkMemberName(str, "Method")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dsd, textEdit, "Method", twItem2);
                                     if(twItem3){}
                                 }
                            }

                         ds = end_dsd - 1;
                        }
                       //D--S--M
                       if(chkMemberName(str, "Method")){
                            QTreeWidgetItem *twItem2 = addChildItem(ds, textEdit, "Method", twItem1);
                            if(twItem2){}
                        }
                   }

                d = end_ds - 1;

               }

               //D--D
               if(chkMemberName(str, "Device"))
               {
                   QTreeWidgetItem *twItem1 = addChildItem(d, textEdit, "Device", twItem0);
                   if(twItem1){}

                   int start_dd = d + 1;
                   int end_dd = getBraceScope(start_dd, count, textEdit);
                   for(int dd = start_dd; dd < end_dd; dd ++){
                       if(break_run){break;}
                       QString str = textEdit->text(dd);

                       //D--D--S
                       if(chkMemberName(str, "Scope")){
                            QTreeWidgetItem *twItem2 = addChildItem(dd, textEdit, "Scope", twItem1);
                            if(twItem2){}

                            /*下一个子层*/
                            int start_dds = dd + 1;
                            int end_dds = getBraceScope(start_dds, count, textEdit);
                            for(int dds = start_dds; dds < end_dds; dds ++){
                                if(break_run){break;}
                                QString str = textEdit->text(dds);

                                //D--D--S--S
                                if(chkMemberName(str, "Scope")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dds, textEdit, "Scope", twItem2);
                                     if(twItem3){}

                                     /*下一个子层*/
                                     int start_ddss = dds + 1;
                                     int end_ddss = getBraceScope(start_ddss, count, textEdit);
                                     for(int ddss = start_ddss; ddss < end_ddss; ddss ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(ddss);

                                         //D--D--S--S--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddss, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--S--S--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddss, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--S--S--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddss, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  dds = end_ddss - 1;
                                 }
                                //D--D--S--D
                                if(chkMemberName(str, "Device")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dds, textEdit, "Device", twItem2);
                                     if(twItem3){}

                                     /*下一个子层*/
                                     int start_ddsd = dds + 1;
                                     int end_ddsd = getBraceScope(start_ddsd, count, textEdit);
                                     for(int ddsd = start_ddsd; ddsd < end_ddsd; ddsd ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(ddsd);

                                         //D--D--S--D--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddsd, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--S--D--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddsd, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--S--D--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddsd, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  dds = end_ddsd - 1;
                                 }
                                //D--D--S--M
                                if(chkMemberName(str, "Method")){
                                     QTreeWidgetItem *twItem3 = addChildItem(dds, textEdit, "Method", twItem2);
                                     if(twItem3){}
                                 }
                            }

                         dd = end_dds - 1;
                        }

                       //D--D--D
                       if(chkMemberName(str, "Device")){
                            QTreeWidgetItem *twItem2 = addChildItem(dd, textEdit, "Device", twItem1);
                            if(twItem2){}

                            int start_ddd = dd + 1;
                            int end_ddd = getBraceScope(start_ddd, count, textEdit);
                            for(int ddd = start_ddd; ddd < end_ddd; ddd ++){
                                if(break_run){break;}
                                QString str = textEdit->text(ddd);

                                //D--D--D--S
                                if(chkMemberName(str, "Scope")){
                                     QTreeWidgetItem *twItem3 = addChildItem(ddd, textEdit, "Scope", twItem2);
                                     if(twItem3){}

                                     int start_ddds = ddd + 1;
                                     int end_ddds = getBraceScope(start_ddds, count, textEdit);
                                     for(int ddds = start_ddds; ddds < end_ddds; ddds ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(ddds);

                                         //D--D--D--S--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddds, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--D--S--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddds, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--D--S--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(ddds, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  ddd = end_ddds - 1;
                                 }
                                //D--D--D--D
                                if(chkMemberName(str, "Device")){
                                     QTreeWidgetItem *twItem3 = addChildItem(ddd, textEdit, "Device", twItem2);
                                     if(twItem3){}

                                     int start_dddd = ddd + 1;
                                     int end_dddd = getBraceScope(start_dddd, count, textEdit);
                                     for(int dddd = start_dddd; dddd < end_dddd; dddd ++){
                                         if(break_run){break;}
                                         QString str = textEdit->text(dddd);

                                         //D--D--D--D--S
                                         if(chkMemberName(str, "Scope")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dddd, textEdit, "Scope", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--D--D--D
                                         if(chkMemberName(str, "Device")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dddd, textEdit, "Device", twItem3);
                                              if(twItem4){}
                                          }
                                         //D--D--D--D--M
                                         if(chkMemberName(str, "Method")){
                                              QTreeWidgetItem *twItem4 = addChildItem(dddd, textEdit, "Method", twItem3);
                                              if(twItem4){}
                                          }
                                     }

                                  ddd = end_dddd - 1;
                                 }
                                //D--D--D--M
                                if(chkMemberName(str, "Method")){
                                     QTreeWidgetItem *twItem3 = addChildItem(ddd, textEdit, "Method", twItem2);
                                     if(twItem3){}
                                 }
                            }

                         dd = end_ddd - 1;
                        }

                       //D--D--M
                       if(chkMemberName(str, "Method")){
                            QTreeWidgetItem *twItem2 = addChildItem(dd, textEdit, "Method", twItem1);
                            if(twItem2){}
                        }
                   }

                d = end_dd - 1;

               }

               //D--M
               if(chkMemberName(str, "Method"))
               {
                   QTreeWidgetItem *twItem1 = addChildItem(d, textEdit, "Method", twItem0);
                   if(twItem1){}

               }

           }

           j = end_d -1;


        }


    }


    loading = false;

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

            break;
        }

        //正则、区分大小写、匹配整个单词、循环搜索
        if(textEdit->findFirst(str_member , true , true , true , false))
        {

           textEdit->getCursorPosition(&RowNum, &ColNum);

           str = textEdit->text(RowNum);

           QString space = findKey(str, str_member.mid(0, 1), 0);

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
                           QFont f;
                           f.setBold(true);
                           twItem0->setFont(0, f);

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

    ui->tabWidget->setMaximumHeight(220);

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

    connect(ui->actionKextstat, &QAction::triggered, this, &MainWindow::kextstat);

    QIcon icon;
    icon.addFile(":/icon/1.png");
    ui->btnPreviousError->setIcon(icon);

    icon.addFile(":/icon/2.png");
    ui->btnCompile->setIcon(icon);

    icon.addFile(":/icon/3.png");
    ui->btnNextError->setIcon(icon);

    icon.addFile(":/icon/return.png");
    ui->btnReturn->setIcon(icon);

    ui->cboxCompilationOptions->addItem("-f");
    ui->cboxCompilationOptions->addItem("-tp");
    ui->cboxCompilationOptions->setEditable(true);

    //设置编译功能屏蔽
    ui->actionCompiling->setEnabled(false);
    ui->btnCompile->setEnabled(false);


}

void MainWindow::setLexer(QsciLexer *textLexer)
{


    //获取背景色
    QPalette pal = this->palette();
    QBrush brush = pal.window();
    red = brush.color().red();

    if(red < 55) //暗模式，mac下为50
    {

        //背景色
        //textLexer->setPaper(QColor(28, 28, 28));

        //设置光标所在行背景色
        textEdit->setCaretLineBackgroundColor(QColor(180, 180, 0));
        textEdit->setCaretLineFrameWidth(1);
        textEdit->setCaretLineVisible(true);

        textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::CommentLine);//"//"注释颜色
        textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::Comment);

        textLexer->setColor(QColor(210, 210, 210 ), QsciLexerCPP::Identifier);
        textLexer->setColor(QColor(245, 150, 147), QsciLexerCPP::Number);
        textLexer->setColor(QColor(100, 100, 250), QsciLexerCPP::Keyword);
        textLexer->setColor(QColor(210, 32, 240 ), QsciLexerCPP::KeywordSet2);
        textLexer->setColor(QColor(245, 245, 245 ), QsciLexerCPP::Operator);
        textLexer->setColor(QColor(84, 235, 159 ), QsciLexerCPP::DoubleQuotedString);//双引号
     }
    else
    {

        //背景色
        //textLexer->setPaper(QColor(255, 255, 255));

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
#ifdef Q_OS_WIN32
  textEdit->setMarginWidth(0, 70);
#endif

#ifdef Q_OS_LINUX
  textEdit->setMarginWidth(0, 60);
#endif

#ifdef Q_OS_MAC
   textEdit->setMarginWidth(0, 60);
#endif

    textEdit->setMarginType(0, QsciScintilla::NumberMargin);

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

    /*断点设置区域,为后面可能会用到的功能预留*/
    /*textEdit->setMarginType(1, QsciScintilla::SymbolMargin);
    textEdit->setMarginLineNumbers(1, false);
    textEdit->setMarginWidth(1,20);
    textEdit->setMarginSensitivity(1, true);    //设置是否可以显示断点
    textEdit->setMarginsBackgroundColor(QColor("#bbfaae"));
    textEdit->setMarginMarkerMask(1, 0x02);
    connect(textEdit, SIGNAL(marginClicked(int, int, Qt::KeyboardModifiers)),this,
            SLOT(on_margin_clicked(int, int, Qt::KeyboardModifiers)));
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

    //读取字体和编译参数
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

        QString op = Reg.value("options").toString().trimmed();
        if(op.count() > 0)
            ui->cboxCompilationOptions->setCurrentText(op);

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
    treeWidgetBack->setColumnHidden(1 , true);
    treeWidgetBack->setColumnCount(2);

    treeWidgetBack->setColumnWidth(0 , w/3);
    treeWidgetBack->setColumnWidth(1 , 100);
    treeWidgetBack->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("Members") << "Lines"));

    //设置水平滚动条
    treeWidgetBack->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeWidgetBack->header()->setStretchLastSection(false);

    treeWidgetBack->setFont(font);

    ui->treeWidget->setStyle(QStyleFactory::create("windows")); //连接的虚线
    ui->treeWidget->setIconSize(QSize(12, 12));

    ui->treeWidget->installEventFilter(this);
    //ui->treeWidget->setAlternatingRowColors(true);//底色交替显示
    //ui->treeWidget->setStyleSheet( "QTreeWidget::item:hover{background-color:rgb(0,0,255,20)}" "QTreeWidget::item:selected{background-color:rgb(135, 206, 255)}" );

}

void MainWindow::init_filesystem()
{

    ui->treeView->installEventFilter(this);//安装事件过滤器
    //ui->treeView->setAlternatingRowColors(true);//不同的底色交替显示

    model = new QFileSystemModel;
    model->setRootPath("/Volumes");

    ui->treeView->setModel(model);

    ui->treeView->setAnimated(false);
    ui->treeView->setIndentation(20);
    ui->treeView->setSortingEnabled(true);
    const QSize availableSize = ui->treeView->screen()->availableGeometry().size();
    ui->treeView->resize(availableSize / 2);
    ui->treeView->setColumnWidth(0, ui->treeView->width() / 3);

    ui->tabWidget_misc->setCurrentIndex(0);


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
        ui->tabWidget->setTabText(1, tr("Errors") + " (" + QString::number(info_count) +")");
    if(str_key == "Warning")
        ui->tabWidget->setTabText(2, tr("Warnings") + " (" + QString::number(info_count) +")");
    if(str_key == "Remark")
        ui->tabWidget->setTabText(3, tr("Remarks") + " (" + QString::number(info_count) +")");
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

    //存储编译选项
    QString qfile = QDir::homePath() + "/QtiASL.ini";
    QFile file(qfile);
    //QSettings Reg(qfile, QSettings::NativeFormat);
    QSettings Reg(qfile, QSettings::IniFormat);//全平台都采用ini格式
    Reg.setValue("options" , ui->cboxCompilationOptions->currentText().trimmed());


    if(textEdit->isModified())
    {

        int choice;
        if(!zh_cn)
        {

            choice = QMessageBox::warning(this, tr("Application"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        }
        else
        {
            QMessageBox message(QMessageBox::Warning,"QtiASL","文件内容已修改，是否保存？");
            message.setStandardButtons (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            message.setButtonText (QMessageBox::Save,QString("保 存"));
            message.setButtonText (QMessageBox::Cancel,QString("取 消"));
            message.setButtonText (QMessageBox::Discard,QString("放 弃"));
            message.setDefaultButton(QMessageBox::Save);
            choice = message.exec();

        }


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
        break_run = true;
        mythread->quit();
        mythread->wait();
    }


}

void MainWindow::recentOpen(QString filename)
{

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
    //状态栏
    lblLayer = new QLabel(this);
    QPalette label_palette;
    label_palette.setColor(QPalette::Background, QColor(0, 0, 200));
    label_palette.setColor(QPalette::WindowText,Qt::white);
    lblLayer->setAutoFillBackground(true);
    lblLayer->setPalette(label_palette);
    lblLayer->setText(tr(" Layer "));
    lblLayer->setTextInteractionFlags(Qt::TextSelectableByMouse); //允许选择其中的文字

    editLayer = new QLineEdit();
    editLayer->setReadOnly(true);
    ui->statusbar->addPermanentWidget(lblLayer);

    lblMsg = new QLabel(this);
    ui->statusbar->addPermanentWidget(lblMsg);


}

void MainWindow::newFile()
{

    if(!thread_end)
    {
        break_run = true; //通知打断线程
        mythread->quit();
        mythread->wait();

    }

    loading = true;

    if(maybeSave())
    {

        ui->treeWidget->clear();
        s_count = 0;
        d_count = 0;
        m_count = 0;
        ui->treeWidget->setHeaderLabel("Scope(" + QString::number(s_count) + ")  " + "Device(" + QString::number(d_count) + ")  " + "Method(" + QString::number(m_count) + ")");

        textEdit->clear();
        textEditBack->clear();

        ui->editShowMsg->clear();
        ui->editErrors->clear();
        ui->editWarnings->clear();
        ui->editRemarks->clear();
        setCurrentFile("");//通过新建的untitled.dsl文件来刷新treeWidget，无需对tw进行任何操作

        lblLayer->setText("");
        lblMsg->setText("");

        ui->treeWidget->setHidden(false);

    }

    loading = false;
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
    on_btnFindNext_clicked();
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

            QPalette palette;
            palette = ui->editFind->palette();
            palette.setColor(QPalette::Base, QColor(50,50,50));
            ui->editFind->setPalette(palette);

        }
        else
        {

            QPalette palette;
            palette = ui->editFind->palette();
            palette.setColor(QPalette::Base, Qt::white);
            ui->editFind->setPalette(palette);


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
        /*目前暂时停用。需要解决：1.代码折叠线的颜色 2.双引号输入时的背景色*/
        //setLexer(textLexer);
        //textEdit->repaint();

    }


}

int MainWindow::treeCount(QTreeWidget *tree, QTreeWidgetItem *parent)
{
    Q_ASSERT(tree != NULL);

    int count = 0;
    if (parent == 0) {
        int topCount = tree->topLevelItemCount();
        for (int i = 0; i < topCount; i++) {
            QTreeWidgetItem *item = tree->topLevelItem(i);
            if (item->isExpanded()) {
                count += treeCount(tree, item);
            }
        }
        count += topCount;
    } else {
        int childCount = parent->childCount();
        for (int i = 0; i < childCount; i++) {
            QTreeWidgetItem *item = parent->child(i);
            if (item->isExpanded()) {
                count += treeCount(tree, item);
            }
        }
        count += childCount;
    }
    return count;
}

int MainWindow::treeCount(QTreeWidget *tree)
{
    Q_ASSERT(tree != NULL);

    int count = 0;

        int topCount = tree->topLevelItemCount();
        for (int i = 0; i < topCount; i++) {
            QTreeWidgetItem *item = tree->topLevelItem(i);
            if (item->isExpanded()) {
                count += treeCount(tree, item);
            }
        }
        count += topCount;

    return count;
}

/*获取当前条目的所有上级条目*/
QString MainWindow::getLayerName(QTreeWidgetItem *hItem)
{
       if(!hItem)
          return "";

        QString str0 = hItem->text(0); //记录初始值，即为当前被选中的条目值
        QString str;
        char sername[255];
        memset(sername, 0, 255);
        QVector<QString> list;
        QTreeWidgetItem * phItem = hItem->parent();    //获取当前item的父item
        if(!phItem)
          {	    // 根节点
                QString  qstr = hItem->text(0);
                QByteArray ba = qstr.toLatin1();  //实现QString和 char *的转换
                const char *cstr = ba.data();
                strcpy(sername, cstr);
          }
            else{
                while (phItem)
                {

                    QString  qstr = phItem->text(0);
                    QByteArray ba = qstr.toLatin1();    //实现QString和char *的转换
                    const char *cstr = ba.data();
                    strcpy(sername, cstr);
                    phItem = phItem->parent();

                    list.push_back(sername);
                }

            }
    for(int i = 0; i < list.count(); i ++)
    {
        str = list.at(i) + " --> " + str;
    }

    return " " + str + str0 + " ";


}

void MainWindow::kextstat()
{
    pk = new QProcess;
    pk->start("kextstat");
    connect(pk , SIGNAL(finished(int)) , this , SLOT(readKextstat()));
}

void MainWindow::readKextstat()
{
    QString result = QString::fromUtf8(pk->readAll());
    newFile();
    textEdit->append(result);
    ui->treeWidget->setHidden(true);
    ui->tabWidget->setHidden(true);
}

void MainWindow::loadLocal()
{
       QTextCodec *codec = QTextCodec::codecForName("System");
       QTextCodec::setCodecForLocale(codec);

       static QTranslator translator;  //注意：使translator一直生效
       QLocale locale;
       if( locale.language() == QLocale::English )  //获取系统语言环境
       {

           zh_cn = false;

       }
       else if( locale.language() == QLocale::Chinese )
       {

           bool tr = false;
           tr = translator.load(":/tr/cn.qm");
           if(tr)
           {
               qApp->installTranslator(&translator);
               zh_cn = true;
           }

           ui->retranslateUi(this);
       }

}


void MainWindow::on_btnCompile_clicked()
{
    btnCompile_clicked();
}

void MainWindow::on_treeView_doubleClicked(const QModelIndex &index)
{

    fsm_Filepath = model->filePath(index);
    fsm_Index = index;

    if(!model->isDir(index))
    {
        if(maybeSave())
            loadFile(openFile(fsm_Filepath));
    }

    ui->btnReturn->setText(fsm_Filepath);



}

void MainWindow::on_btnReturn_clicked()
{

    QString str = model->filePath(fsm_Index.parent());

    ui->treeView->setRootIndex(model->index(str));

    ui->btnReturn->setText(str);

    fsm_Index = fsm_Index.parent();


}

void MainWindow::on_treeView_expanded(const QModelIndex &index)
{
    fsm_Index = index;
    QString str = model->filePath(index);
    ui->btnReturn->setText(str);
}

void MainWindow::on_treeView_collapsed(const QModelIndex &index)
{
    fsm_Index = index;
    QString str = model->filePath(index);
    ui->btnReturn->setText(str);
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
     if (watched==ui->treeView)
     {
          if (event->type()==QEvent::FocusIn)
          {

          }
          else if (event->type()==QEvent::FocusOut)
          {

          }
     }

     if (watched==ui->treeWidget)         //判断控件
     {
          if (event->type()==QEvent::FocusIn)     //控件获得焦点事件)
          {
              //ui->treeWidget->setStyleSheet( "QTreeWidget::item:hover{background-color:rgb(0,255,0,0)}" "QTreeWidget::item:selected{background-color:rgb(255,0,5)}" );

          }
          else if (event->type()==QEvent::FocusOut)    //控件失去焦点事件
          {
             //ui->treeWidget->setStyleSheet( "QTreeWidget::item:hover{background-color:rgb(0,255,0,0)}" "QTreeWidget::item:selected{background-color:rgb(255,0,0)}" );
          }
     }

     return QWidget::eventFilter(watched,event);

}
