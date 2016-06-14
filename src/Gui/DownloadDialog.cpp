/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QPushButton>
# include <QHBoxLayout>
# include <QNetworkAccessManager>
# include <QNetworkReply>
# include <QNetworkRequest>
# include <QUrl>
# include <QSslError>
#endif

#include <QAuthenticator>
#include "DownloadDialog.h"
#include "ui_DlgAuthorization.h"
#include <Base/Console.h>

using namespace Gui::Dialog;


DownloadDialog::DownloadDialog(const QUrl& url, QWidget *parent)
  : QDialog(parent), url(url), file(NULL), _qnr(NULL)
{
    _qnam = new QNetworkAccessManager(this);
    connect(_qnam, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), this, SLOT(onAuthenticationRequired(QNetworkReply *, QAuthenticator *)));

    statusLabel = new QLabel(url.toString());
    progressBar = new QProgressBar(this);
    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setDefault(true);
    cancelButton = new QPushButton(tr("Cancel"));
    closeButton = new QPushButton(tr("Close"));
    closeButton->setAutoDefault(false);

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(closeButton, QDialogButtonBox::RejectRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
    cancelButton->hide();

    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelDownload()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(statusLabel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Download"));
}

DownloadDialog::~DownloadDialog()
{
}

void DownloadDialog::downloadFile()
{
    QFileInfo fileInfo(url.path());
    QString fileName = fileInfo.fileName();
    if (QFile::exists(fileName)) {
        if (QMessageBox::question(this, tr("Download"),
                tr("There already exists a file called %1 in "
                   "the current directory. Overwrite?").arg(fileName),
                QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No)
            return;
        QFile::remove(fileName);
    }

    if (file) {
        file->close();
        file->remove();
        file->deleteLater();
        file = NULL;
    }
    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Download"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        file->deleteLater();
        file = NULL;
        return;
    }

    QNetworkRequest request;
    request.setUrl(url);

    qNetworkReplyAborted = false;

    if(_qnr) {
        disconnect(_qnr);
        _qnr->deleteLater();
    }
    _qnr = _qnam->get(request);
    connect(_qnr, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(_qnr, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(onError()));
    connect(_qnr, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(onSslErrors(QList<QSslError>)));
    connect(_qnr, SIGNAL(downloadProgress(qint64 , qint64 )), this, SLOT(onDownloadProgress(qint64 , qint64 )));
    connect(_qnr, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    statusLabel->setText(tr("Downloading %1.").arg(fileName));
    downloadButton->setEnabled(false);
    cancelButton->show();
    closeButton->hide();
}

void DownloadDialog::cancelDownload()
{
    statusLabel->setText(tr("Download canceled."));
    qNetworkReplyAborted = true;
    _qnr->abort();
    close();
}

void DownloadDialog::onError() {
    if(!_qnr)
        return;

    Base::Console().Log("NetworkError %d: %s\n", _qnr->error(), (const char*)_qnr->errorString().toLatin1());
}

void DownloadDialog::onSslErrors(QList<QSslError> qsslErrs) {
    if(!_qnr)
        return;

    for(int i = 0; i < qsslErrs.size(); ++i) {
        QSslError qsslErr = qsslErrs.at(i);
        Base::Console().Log("SslError %d: %s\n", qsslErr.error(), (const char*)qsslErr.errorString().toLatin1());
    }
}

void DownloadDialog::onFinished()
{
    if (qNetworkReplyAborted) {
        if (file) {
            file->close();
            file->remove();
            file->deleteLater();
            file = NULL;
        }

        progressBar->hide();
        return;
    }

    // There might be data left
    QDataStream writeStream(file);
    writeStream << _qnr->readAll();

    progressBar->hide();
    file->close();

    if (_qnr->error() != QNetworkReply::NoError) {
        file->remove();
        QMessageBox::information(this, tr("Download"),
                                 tr("Download failed: %1.")
                                 .arg(_qnr->errorString()));
    }
    else {
        QString fileName = QFileInfo(url.path()).fileName();
        statusLabel->setText(tr("Downloaded %1 to current directory.").arg(fileName));
    }

    downloadButton->setEnabled(true);
    cancelButton->hide();
    closeButton->show();
    file->deleteLater();
    file = NULL;
    disconnect(_qnr);
    _qnr->deleteLater();
    _qnr = NULL;
}

void DownloadDialog::onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    QDialog dlg;
    Ui_DlgAuthorization ui;
    ui.setupUi(&dlg);
    dlg.adjustSize();
    ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm()).arg(reply->url().host()));

    if (dlg.exec() == QDialog::Accepted) {
        authenticator->setUser(ui.username->text());
        authenticator->setPassword(ui.password->text());
    }
}

void DownloadDialog::onDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (qNetworkReplyAborted)
        return;

    progressBar->setMaximum(totalBytes);
    progressBar->setValue(bytesRead);
}

void DownloadDialog::onReadyRead()
{
    if (qNetworkReplyAborted)
        return;

    QDataStream writeStream(file);
    writeStream << _qnr->readAll();
}

#include "moc_DownloadDialog.cpp"
