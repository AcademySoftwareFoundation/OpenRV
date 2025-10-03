//******************************************************************************
// Copyright (c) 2025 Autodesk.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include "QAlertPanel.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>
#include <QtCore/QTimer>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <iostream>

QAlertPanel::QAlertPanel(QWidget* parent)
    : QDialog(parent)
    , m_iconLabel(nullptr)
    , m_textLabel(nullptr)
    , m_clickedButton(nullptr)
    , m_mainLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_contentLayout(nullptr)
    , m_icon(NoIcon)
{
    setupUI();
}

QAlertPanel::~QAlertPanel() {}

void QAlertPanel::setupUI()
{
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint
                   | Qt::WindowCloseButtonHint);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);

    // Content layout (icon + text)
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setSpacing(15);

    // Icon label
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(32, 32);
    m_iconLabel->setScaledContents(true);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_contentLayout->addWidget(m_iconLabel);

    // Text label
    m_textLabel = new QLabel();
    m_textLabel->setWordWrap(true);
    m_textLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_textLabel->setMinimumSize(0, 0); // Allow it to shrink
    m_contentLayout->addWidget(m_textLabel, 1);

    m_mainLayout->addLayout(m_contentLayout);

    // Button layout
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(10);
    m_buttonLayout->addStretch();
    m_mainLayout->addLayout(m_buttonLayout);

    // Set default icon
    updateIcon();

    // Set minimum size and enable auto-sizing
    setMinimumSize(300, 120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void QAlertPanel::setText(const QString& text)
{
    m_text = text;
    m_textLabel->setText(text);

    // Don't resize here - let exec() handle the sizing
}

void QAlertPanel::setIcon(IconType icon)
{
    m_iconType = icon;
    updateIcon();
}

QPushButton* QAlertPanel::addButton(const QString& text, ButtonRole role)
{
    QPushButton* button = new QPushButton(text);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setDefault(false);
    button->setAutoDefault(false);

    // Set button as default based on role
    if (role == AcceptRole)
    {
        button->setDefault(true);
        button->setAutoDefault(true);
    }

    m_buttonLayout->addWidget(button);

    m_roleToButton[role] = button;

    // Connect button click to our handler
    connect(button, &QPushButton::clicked, this, &QAlertPanel::onButtonClicked);

    // Store reference for clickedButton()
    connect(button, &QPushButton::clicked,
            [this, button]() { m_clickedButton = button; });

    return button;
}

QPushButton* QAlertPanel::clickedButton() const { return m_clickedButton; }

int QAlertPanel::exec()
{
    // Adjust dialog size for optimal text wrapping
    adjustBestFitDimensions();

    // Ensure the dialog is properly centered and visible
    if (parentWidget())
    {
        move(parentWidget()->geometry().center() - rect().center());
    }

    // Make sure buttons are focusable and the first button gets focus
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    if (!buttons.isEmpty())
    {
        buttons.first()->setFocus();
    }

    return QDialog::exec();
}

void QAlertPanel::adjustBestFitDimensions()
{
    // Calculate optimal size based on text content for nice text wrapping
    if (!m_text.isEmpty())
    {
        // Get the text size with word wrapping at a reasonable width
        QFontMetrics fm(m_textLabel->font());

        // Try different widths to find the best text wrapping
        float bestFitRatio = 1e100;
        int bestWidth = 100; // Start with a reasonable width
        int bestHeight = 50;

        // float targetBestFitRatio = 8.0/2.0;
        float targetBestFitRatio = 2.0;

        // Test widths from 100 to 1000 pixels to find optimal wrapping
        for (int testWidth = 100; testWidth <= 1000; testWidth += 20)
        {
            QRect textRect = fm.boundingRect(QRect(0, 0, testWidth, 0),
                                             Qt::TextWordWrap, m_text);

            float fitRatio = testWidth / (float)textRect.height();

            // Only consider ratios that are >= targetBestFitRatio
            if (fitRatio >= targetBestFitRatio)
            {
                float ratioDifference = fabs(fitRatio - targetBestFitRatio);

                if (ratioDifference < fabs(bestFitRatio - targetBestFitRatio))
                {
                    bestFitRatio = fitRatio;
                    bestWidth = testWidth;
                    bestHeight = textRect.height();
                }
            }
        }

        // Calculate dialog size with reasonable padding
        int dialogWidth = bestWidth + 80; // Add padding for icon and margins
        int dialogHeight =
            bestHeight + 80; // Add padding for buttons and margins

        // Set minimum size to calculated dimensions, but allow expansion
        setMinimumSize(dialogWidth, dialogHeight);

        // Set the calculated size as initial size
        resize(dialogWidth, dialogHeight);
    }
    else
    {
        // Default size if no text
        resize(300, 120);
    }
}

void QAlertPanel::onButtonClicked() { accept(); }

void QAlertPanel::updateIcon()
{
    if (!m_iconLabel)
        return;

    QStyle* style = QApplication::style();
    QIcon icon;

    switch (m_icon)
    {
    case Information:
        icon = style->standardIcon(QStyle::SP_MessageBoxInformation);
        break;
    case Warning:
        icon = style->standardIcon(QStyle::SP_MessageBoxWarning);
        break;
    case Critical:
        icon = style->standardIcon(QStyle::SP_MessageBoxCritical);
        break;
    case NoIcon:
    default:
        m_iconLabel->setVisible(false);
        return;
    }

    m_iconLabel->setPixmap(icon.pixmap(32, 32));
    m_iconLabel->setVisible(true);
}

void QAlertPanel::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
    {
        // Handle Tab navigation between buttons
        QList<QPushButton*> buttons = findChildren<QPushButton*>();
        if (buttons.size() <= 1)
        {
            QDialog::keyPressEvent(event);
            return;
        }

        QPushButton* currentButton = qobject_cast<QPushButton*>(focusWidget());
        int currentIndex = buttons.indexOf(currentButton);

        if (event->key() == Qt::Key_Tab)
        {
            // Forward tab
            int nextIndex = (currentIndex + 1) % buttons.size();
            buttons[nextIndex]->setFocus();
        }
        else
        {
            // Backward tab
            int nextIndex =
                (currentIndex - 1 + buttons.size()) % buttons.size();
            buttons[nextIndex]->setFocus();
        }

        event->accept();
        return;
    }

    // Handle Enter and Escape keys
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        QPushButton* currentButton = qobject_cast<QPushButton*>(focusWidget());
        if (currentButton)
        {
            currentButton->click();
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_Escape)
    {
        QPushButton* rejectButton = m_roleToButton.value(RejectRole, nullptr);

        if (rejectButton)
        {
            rejectButton
                ->click(); // This will set m_clickedButton and call accept()
            event->accept();
            return;
        }
    }

    QDialog::keyPressEvent(event);
}

#include "QAlertPanel.moc"
