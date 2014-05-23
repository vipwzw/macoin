// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>

namespace Ui {
    class LoginDialog;
}
class WalletModel;
class OptionsModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog for requesting payment of bitcoins */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

    void setModel(WalletModel *model);

public slots:
    void clear();
    void reject();
    void accept();

protected:
    virtual void keyPressEvent(QKeyEvent *event);


private:
    Ui::LoginDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;
    void copyColumnToClipboard(int column);

private slots:
    void on_loginButton_clicked();
	void on_logoutButton_clicked();
	void on_subscriptButton_clicked();
    void on_showRequestButton_clicked();
    void on_removeRequestButton_clicked();
    void on_recentRequestsView_doubleClicked(const QModelIndex &index);
    void updateDisplayUnit();
    void showMenu(const QPoint &);
    void copyLabel();
    void copyMessage();
    void copyAmount();

	void showLoginView();
signals:
    void displayLoginView();
};

#endif // LOGINDIALOG_H
