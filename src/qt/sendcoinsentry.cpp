// Copyright (c) 2011-2013 The Macoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsentry.h"
#include "ui_sendcoinsentry.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QApplication>
#include <QClipboard>

 
#include "rpcserver.h"
#include "rpcclient.h"

#include "base58.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace json_spirit;



void GetSMSThread::startwork()  
{  
	start(); //HighestPriority 
}  


int GetSMSThread::GetSMSCode()
{
		try{
			const Object reto = Macoin::sendRandCode();
			
			Value errorObj = find_value(reto,  "error");
			if (errorObj.type() == null_type)
			{
				return 1;
			}
			const string strerror = errorObj.get_str(); 
			
		}catch(QString exception){
			return 2 ;
		}
		return 0;
}

void GetSMSThread::run()  
{ 
	int ret =-1;
	switch(m_type){
		case 4:
		{
			ret = GetSMSCode();
			emit notify(ret);  
			break;
		}
	}
}  




SendCoinsEntry::SendCoinsEntry(QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::SendCoinsEntry),
    model(0)
{
    ui->setupUi(this);

    setCurrentWidget(ui->SendCoins);

#ifdef Q_OS_MAC
    ui->payToLayout->setSpacing(4);
#endif
#if QT_VERSION >= 0x040700
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
#endif

    // normal bitcoin address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    // just a label for displaying bitcoin address(es)
    ui->payTo_is->setFont(GUIUtil::bitcoinAddressFont());
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

void SendCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void SendCoinsEntry::on_payTo_textChanged(const QString &address)
{
    updateLabel(address);
}

void SendCoinsEntry::setModel(WalletModel *model)
{
    this->model = model;

    if (model && model->getOptionsModel())
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    connect(ui->payAmount, SIGNAL(textChanged()), this, SIGNAL(payAmountChanged()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));
	//connect(ui->checkButton , SIGNAL(clicked()), this, SLOT(SMSCheck()));

    clear();
}

void SendCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->payAmount->clear();
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    // clear UI elements for insecure payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    ui->payAmount_is->clear();
    // clear UI elements for secure payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void SendCoinsEntry::deleteClicked()
{
    emit removeEntry(this);
}

void SendCoinsEntry::GetSMSCode()
{
		try{
			const Object reto = Macoin::sendRandCode();
			
			Value errorObj = find_value(reto,  "error");
			if (errorObj.type() == null_type)
			{
				QMessageBox::warning(this, "macoin",
					tr("sms sending complete!"),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
				return ;
			}
			const string strerror = errorObj.get_str(); 
			  QMessageBox::warning(this, "macoin",
					QString::fromStdString(strerror),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
			
		}catch(QString exception){
			QMessageBox::warning(this, "macoin",
                exception,
                QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
		return ;
}

void SendCoinsEntry::on_checkButton_clicked()
{
	if(OAuth2::getAccessToken() != ""){

		 render = new GetSMSThread(4);
		 connect(render,SIGNAL(notify(int)),this,SLOT(OnNotify(int)));  
		 ui->checkButton->setEnabled(false);
		 render->startwork();    

	}else{
		  QMessageBox::warning(this, "macoin",
                (tr("please login first!")),
                QMessageBox::Ok, QMessageBox::Ok);
	}
}


void SendCoinsEntry::OnNotify(int type)  
{  
	switch (type)
	{
		case 0:
		{
			  QMessageBox::warning(this, "macoin",
					QString::fromStdString("sms send fail"),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
				break;
		case 1:
		{
				QMessageBox::warning(this, "macoin",
					tr("sms sending complete!"),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
			 break;
		case 2:
		{
			QMessageBox::warning(this, "macoin",
                "sms send exception",
                QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
				break;
	}
}

bool SendCoinsEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    if (!model->validateAddress(ui->payTo->text()))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if (!ui->payAmount->validate())
    {
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(ui->payTo->text(), ui->payAmount->value())) {
        ui->payAmount->setValid(false);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();
    recipient.message = ui->messageTextLabel->text();
    recipient.smsverifycode = ui->checkSMS->text();
	recipient.stramount = ui->payAmount->text();
    return recipient;
}

QWidget *SendCoinsEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
    QWidget *w = ui->payAmount->setupTabChain(ui->addAsLabel);
    QWidget::setTabOrder(w, ui->addressBookButton);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    return ui->deleteButton;
}

void SendCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    recipient = value;

    if (recipient.paymentRequest.IsInitialized()) // payment request
    {
        if (recipient.authenticatedMerchant.isEmpty()) // insecure
        {
            ui->payTo_is->setText(recipient.address);
            ui->memoTextLabel_is->setText(recipient.message);
            ui->payAmount_is->setValue(recipient.amount);
            ui->payAmount_is->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_InsecurePaymentRequest);
        }
        else // secure
        {
            ui->payTo_s->setText(recipient.authenticatedMerchant);
            ui->memoTextLabel_s->setText(recipient.message);
            ui->payAmount_s->setValue(recipient.amount);
            ui->payAmount_s->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_SecurePaymentRequest);
        }
    }
    else // normal payment
    {
        // message
        ui->messageTextLabel->setText(recipient.message);
        ui->messageTextLabel->setVisible(!recipient.message.isEmpty());
        ui->messageLabel->setVisible(!recipient.message.isEmpty());

        ui->payTo->setText(recipient.address);
        ui->addAsLabel->setText(recipient.label);
        ui->payAmount->setValue(recipient.amount);
    }
}

void SendCoinsEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);
    ui->payAmount->setFocus();
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

bool SendCoinsEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}
