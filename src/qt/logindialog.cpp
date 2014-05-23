// Copyright (c) 2011-2014 The Macoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logindialog.h"
#include "ui_logindialog.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "receiverequestdialog.h"
#include "addresstablemodel.h"
#include "recentrequeststablemodel.h"

#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QTextDocument>
#include <QScrollBar>

 
#include "rpcserver.h"
#include "rpcclient.h"

#include "base58.h"

#include <boost/algorithm/string.hpp>



using namespace std;
using namespace json_spirit;




LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    model(0)
{
    ui->setupUi(this);

	ui->passwordLabel->setEchoMode (QLineEdit::Password);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    //ui->clearButton->setIcon(QIcon());
    ui->loginButton->setIcon(QIcon());
    //ui->showRequestButton->setIcon(QIcon());
    //ui->removeRequestButton->setIcon(QIcon());
#endif

    // context menu actions
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    //QAction *copyMessageAction = new QAction(tr("Copy message"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);

    // context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyLabelAction);
    //contextMenu->addAction(copyMessageAction);
    contextMenu->addAction(copyAmountAction);

    // context menu signals
    //connect(ui->recentRequestsView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    //connect(copyMessageAction, SIGNAL(triggered()), this, SLOT(copyMessage()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
	
	connect(ui->SubscriptButton, SIGNAL(clicked()), this, SLOT(on_subscriptButton_clicked()));

    //connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
}

void LoginDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        //connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        //updateDisplayUnit();

        //ui->recentRequestsView->setModel(model->getRecentRequestsTableModel());
        //ui->recentRequestsView->setAlternatingRowColors(true);
        //ui->recentRequestsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        //ui->recentRequestsView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        //ui->recentRequestsView->horizontalHeader()->resizeSection(RecentRequestsTableModel::Date, 130);
        //ui->recentRequestsView->horizontalHeader()->resizeSection(RecentRequestsTableModel::Label, 120);
#if QT_VERSION < 0x050000
        //ui->recentRequestsView->horizontalHeader()->setResizeMode(RecentRequestsTableModel::Message, QHeaderView::Stretch);
#else
        //ui->recentRequestsView->horizontalHeader()->setSectionResizeMode(RecentRequestsTableModel::Message, QHeaderView::Stretch);
#endif
        //ui->recentRequestsView->horizontalHeader()->resizeSection(RecentRequestsTableModel::Amount, 100);

        //model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::clear()
{
    //ui->passwordLabel->clear();
    //ui->usernameLabel->setText("");
    //ui->reqMessage->setText("");
    //ui->reuseAddress->setChecked(false);
    //updateDisplayUnit();
}

void LoginDialog::reject()
{
    clear();
}

void LoginDialog::accept()
{
    clear();
}

void LoginDialog::updateDisplayUnit()
{
    /*if(model && model->getOptionsModel())
    {
        ui->reqAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }*/
}

Value CallRPC(string args)
{
    vector<string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    Array params = RPCConvertValues(strMethod, vArgs);

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        Value result = (*method)(params, false);
        return result;
    }
    catch (Object& objError)
    {
        throw runtime_error(find_value(objError, "message").get_str());
    }
}

void LoginDialog::on_subscriptButton_clicked()
{
	if(OAuth2::getAccessToken() != ""){

		Value addrvalue = CallRPC("getnewaddress");
		//BOOST_CHECK(addrvalue.type() == str_type);
		string addr = addrvalue.get_str();

		const Object addrinfo = CallRPC(string("validateaddress ") + addr).get_obj();
		const string pubkey = find_value(addrinfo, "pubkey").get_str();
		Object multiinfo ;
		try{
			multiinfo = Macoin::addmultisigaddress(pubkey);
		}catch(...){
			  QMessageBox::warning(this, "macoin",
					QString::fromStdString("please login first or checking network!"),
					QMessageBox::Ok, QMessageBox::Ok);
              return;
		}
		if (find_value(multiinfo,  "error").type() != null_type) {
  			  QMessageBox::warning(this, "macoin",
					QString::fromStdString("max 3 address or not login"),
					QMessageBox::Ok, QMessageBox::Ok);
              return;
        } 
		const string pubkey1 = "\"" + find_value(multiinfo, "pubkey1").get_str() + "\"";
		const string pubkey2 = "\"" + find_value(multiinfo, "pubkey2").get_str() + "\"";
		const string multiaddr = find_value(multiinfo, "addr").get_str();
		const string multisigwallet = CallRPC(string("addmultisigaddress 2 ") + "["+pubkey1+","+pubkey2+"]" + " macoin_validate_wallet").get_str();
	}else{
		  QMessageBox::warning(this, "macoin",
                QString::fromStdString("please login first!"),
                QMessageBox::Ok, QMessageBox::Ok);
	}
    //BOOST_CHECK(multiaddr == multisigwallet);		
}

void LoginDialog::on_loginButton_clicked()
{
    if(!model || !model->getOptionsModel() || !model->getAddressTableModel() || !model->getRecentRequestsTableModel())
        return;

    QString address;
    QString strusername = ui->usernameLabel->text();
	QString strpassword = ui->passwordLabel->text();
	if (strusername == "" || strpassword == "")
	{
        QMessageBox::warning(this, "macoin",
                tr("username or password is empty"),
                QMessageBox::Ok, QMessageBox::Ok);
		return ;
	}
	try{
		OAuth2::login(strusername.toStdString() ,strpassword.toStdString());// "cykzl@vip.qq.com", "182764125");
	}catch(...){
        QMessageBox::warning(this, "macoin",
                tr("login server fail."),
                QMessageBox::Ok, QMessageBox::Ok);
		return ;
	}
   if (OAuth2::getAccessToken() == "")
   {
        QMessageBox::warning(this, "macoin",
                tr("login server fail."),
                QMessageBox::Ok, QMessageBox::Ok);
		return ;
   }


   map<string, string> params;
   const Object userinfo = Macoin::api("user/info", params, "GET");
   
   const string UID = "\"" + find_value(userinfo, "id").get_str() + "\"";
   const string phone = "\"" + find_value(userinfo, "mobile").get_str() + "\"";
   const string email = "\"" + find_value(userinfo, "email").get_str() + "\"";
   const string ifverify = "\"" + find_value(userinfo, "ifverify").get_str() + "\"";

	ui->UIDLabel->setText(QString::fromStdString(UID));
	ui->phoneLabel->setText(QString::fromStdString(phone));
	ui->emailLabel->setText(QString::fromStdString(email));
	//ui->addressLabel1->setText(QString::fromStdString(""));
	ui->statusLabel->setText(QString::fromStdString(ifverify));

   ui->frame2->setVisible(false);
}

void LoginDialog::on_recentRequestsView_doubleClicked(const QModelIndex &index)
{
    const RecentRequestsTableModel *submodel = model->getRecentRequestsTableModel();
    ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(submodel->entry(index.row()).recipient);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void LoginDialog::on_showRequestButton_clicked()
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();

    //foreach (QModelIndex index, selection)
    //{
        //on_recentRequestsView_doubleClicked(index);
    //}
}

void LoginDialog::on_removeRequestButton_clicked()
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    //if(selection.empty())
        //return;
    // correct for selection mode ContiguousSelection
    //QModelIndex firstIndex = selection.at(0);
    //model->getRecentRequestsTableModel()->removeRows(firstIndex.row(), selection.length(), firstIndex.parent());
}

void LoginDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        // press return -> submit form
        if (ui->usernameLabel->hasFocus() || ui->passwordLabel->hasFocus() )
        {
            event->ignore();
            on_loginButton_clicked();
            return;
        }
    }

    this->QDialog::keyPressEvent(event);
}

// copy column of selected row to clipboard
void LoginDialog::copyColumnToClipboard(int column)
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    //if(selection.empty())
        //return;
    // correct for selection mode ContiguousSelection
//    QModelIndex firstIndex = selection.at(0);
    //GUIUtil::setClipboard(model->getRecentRequestsTableModel()->data(firstIndex.child(firstIndex.row(), column), Qt::EditRole).toString());
}

// context menu
void LoginDialog::showMenu(const QPoint &point)
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    //if(selection.empty())
        //return;
    //contextMenu->exec(QCursor::pos());
}

// context menu action: copy label
void LoginDialog::copyLabel()
{
    //copyColumnToClipboard(RecentRequestsTableModel::Label);
}

// context menu action: copy message
void LoginDialog::copyMessage()
{
    //copyColumnToClipboard(RecentRequestsTableModel::Message);
}

// context menu action: copy amount
void LoginDialog::copyAmount()
{
    //copyColumnToClipboard(RecentRequestsTableModel::Amount);
}
