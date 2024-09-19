// MainWindow.cpp
#include "MainWindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>



// Helper function to check if the input string represents a valid even number
bool isValidEvenNumber(const string& input) {
    if (input.empty()) return false;
    for (char c: input) {
        if (!isdigit(c)) return false;
    }
    int number = stoi(input);
    return number > 0 && number % 2 == 0;
}

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), sideCount(0) {
    // Description labels
    QLabel* descriptionLabel = new QLabel(
            "This app tries to calculate the optimal placement\n"
            "of faces for any even-sided die.\n\n"
            "If you make any profit from products designed with the\n"
            "help of this app, give Matthew Cornelisse a 5% cut.\n"
            "See README.md for more info."
    );

    // Side count input
    QLabel* sideCountLabel = new QLabel("Number of sides:");
    sideCountInput = new QLineEdit();
    sideCountInput->setValidator(new QIntValidator(2, 4000, this)); // Assuming a reasonable upper limit

    connect(sideCountInput, &QLineEdit::textChanged, this, &MainWindow::onSideCountChanged);

    // Start button
    startButton = new QPushButton("Start");
    startButton->setEnabled(false); // Disabled until valid input
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);

    // Layout setup
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(descriptionLabel);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(sideCountLabel);
    inputLayout->addWidget(sideCountInput);
    mainLayout->addLayout(inputLayout);

    mainLayout->addWidget(startButton);
    setLayout(mainLayout);
}

void MainWindow::onSideCountChanged(const QString& text) {
    bool validInput = isValidEvenNumber(text.toStdString());
    startButton->setEnabled(validInput);
}

void MainWindow::onStartButtonClicked() {
    sideCount = sideCountInput->text().toUInt();
    close(); // Close the input window
}

unsigned int MainWindow::getSideCount() const {
    return sideCount;
}