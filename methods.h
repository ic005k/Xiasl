#ifndef METHODS_H
#define METHODS_H

#include <Qsci/qsciscintilla.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMainWindow>
#include <QMessageBox>
#include <QObject>
#include <QTextBlock>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QWidget>

class Methods : public QObject {
  Q_OBJECT
 public:
  explicit Methods(QObject *parent = nullptr);

  static QStringList getVoidForCpp(QsciScintilla *textEdit);
  static QString loadText(QString textFile);
  static QString getTextEditLineText(QTextEdit *txtEdit, int i);
  static bool isSymbol(QString line);
  static void setColorMatch(int red, QsciLexer *textLexer);
  static void setSearchHistory();
  static void getAllFolds(const QString &foldPath, QStringList &folds);
  static void getAllFiles(const QString &foldPath, QStringList &folds,
                          const QStringList &formats);

  static void init_Margin(QsciScintilla *textEdit);
  static void init_MacVerInfo(QString ver);
  static void TextEditToFile(QTextEdit *txtEdit, QString fileName);
 signals:
};

#endif  // METHODS_H
