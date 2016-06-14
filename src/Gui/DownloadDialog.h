/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef GUI_DOWNLOADDIALOG_H
#define GUI_DOWNLOADDIALOG_H

#include <QtCore>
#include <QDialog>
#include <QUrl>
#include <QMessageBox>
#include <QBuffer>
#include <QLabel>
#include <QProgressBar>
#include <QFileInfo>
#include <QCloseEvent>
#include <QDialogButtonBox>

class QFile;
class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QSslError;

namespace Gui {
namespace Dialog {

 /** Download a resource (file) from the web to a location on the disk
 * 
 */

class GuiExport DownloadDialog : public QDialog
{
    Q_OBJECT

public:
    DownloadDialog(const QUrl& url, QWidget *parent = 0);
    ~DownloadDialog();

private Q_SLOTS:
    void downloadFile();
    void cancelDownload();
    void onError();
    void onSslErrors(QList<QSslError>);
    void onReadyRead();
    void onFinished();
    void onAuthenticationRequired(QNetworkReply *, QAuthenticator *);
    void onDownloadProgress(qint64, qint64);

private:
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QPushButton *downloadButton;
    QPushButton *closeButton;
    QPushButton *cancelButton;
    QDialogButtonBox *buttonBox;

    QUrl url;
    QFile *file;
    bool qNetworkReplyAborted;

    QNetworkAccessManager * _qnam;
    QNetworkReply * _qnr;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DOWNLOADDIALOG_H
