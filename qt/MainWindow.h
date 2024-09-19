// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <array>
#include <QLineEdit>
#include <QPushButton>
#include "Die.h"

class MainWindow : public QWidget {
Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    unsigned int getSideCount() const;

private slots:
    void onStartButtonClicked();
    void onSideCountChanged(const QString& text);

private:
    QLineEdit* sideCountInput;
    QPushButton* startButton;
    unsigned int sideCount;
};

#endif // MAINWINDOW_H
