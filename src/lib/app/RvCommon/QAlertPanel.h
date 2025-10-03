//******************************************************************************
// Copyright (c) 2024 Autodesk.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RvCommon__QAlertPanel__h__
#define __RvCommon__QAlertPanel__h__

#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>

class QAlertPanel : public QDialog
{
    Q_OBJECT

public:
    enum IconType
    {
        NoIcon = 0,
        Information = 1,
        Warning = 2,
        Critical = 3
    };

    enum ButtonRole
    {
        AcceptRole = 0,
        RejectRole = 1,
        ApplyRole = 2
    };

    explicit QAlertPanel(QWidget* parent = nullptr);
    virtual ~QAlertPanel();

    // QMessageBox-compatible interface
    void setText(const QString& text);
    void setIcon(IconType icon);

    QPushButton* addButton(const QString& text, ButtonRole role);
    QPushButton* clickedButton() const;
    QPushButton* defaultButton() const;
    QPushButton* rejectButton() const;
    QPushButton* nextPrevButton(QPushButton* fromButton, bool next) const;

    int exec() override;

protected:
    bool event(QEvent* event) override;
    bool focusNextPrevChild(bool next) override;

private slots:
    void onButtonClicked();

private:
    void setupUI();
    void updateIcon();
    void adjustBestFitDimensions();

    QLabel* m_iconLabel;
    QLabel* m_textLabel;
    QPushButton* m_clickedButton;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_buttonLayout;
    QHBoxLayout* m_contentLayout;

    IconType m_iconType;
    QString m_text;

    QPushButton* m_rejectButton;
};

#endif // __RvCommon__QAlertPanel__h__
