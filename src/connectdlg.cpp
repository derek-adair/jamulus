/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "connectdlg.h"


/* Implementation *************************************************************/
CConnectDlg::CConnectDlg ( const bool bNewShowCompleteRegList,
                           QWidget* parent,
                           Qt::WindowFlags f )
    : QDialog ( parent, f ),
      strCentralServerAddress  ( "" ),
      strSelectedAddress       ( "" ),
      strSelectedServerName    ( "" ),
      bShowCompleteRegList     ( bNewShowCompleteRegList ),
      bServerListReceived      ( false ),
      bServerListItemWasChosen ( false ),
      bListFilterWasActive     ( false ),
      bShowAllMusicians        ( true )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // server list
    lvwServers->setWhatsThis ( "<b>" + tr ( "Server List" ) + ":</b> " + tr (
        "The server list shows a list of available servers which are registered at the "
        "central server. Select a server from the list and press the connect button to "
        "connect to this server. Alternatively, double click a server from "
        "the list to connect to it. If a server is occupied, a list of the "
        "connected musicians is available by expanding the list item. "
        "Permanent servers are shown in bold font." ) + "<br>" + tr (
        "Note that it may take some time to retrieve the server list from the "
        "central server. If no valid central server address is specified in "
        "the settings, no server list will be available." ) );

    lvwServers->setAccessibleName ( tr ( "Server list view" ) );

    // server address
    QString strServAddrH = "<b>" + tr ( "Server Address" ) + ":</b> " + tr (
        "The IP address or URL of the server running the " ) + APP_NAME + tr (
        " server software must be set here. An optional port number can be added after the IP "
        "address or URL using a comma as a separator, e.g, "
        "example.org:" ) +
        QString().setNum ( LLCON_DEFAULT_PORT_NUMBER ) + tr ( ". A list of "
        "the most recent used server IP addresses or URLs is available for "
        "selection." );

    lblServerAddr->setWhatsThis ( strServAddrH );
    cbxServerAddr->setWhatsThis ( strServAddrH );

    cbxServerAddr->setAccessibleName        ( tr ( "Server address edit box" ) );
    cbxServerAddr->setAccessibleDescription ( tr ( "Holds the current server "
        "IP address or URL. It also stores old URLs in the combo box list." ) );

    // filter
    edtFilter->setWhatsThis ( "<b>" + tr ( "Filter" ) + ":</b> " + tr ( "The server "
        "list is filered by the given text. Note that the filter is case insensitive." ) );
    edtFilter->setAccessibleName ( tr ( "Filter edit box" ) );

    // show all mucisians
    chbExpandAll->setWhatsThis ( "<b>" + tr ( "Show All Musicians" ) + ":</b> " + tr (
        "If you check this check box, the musicians of all servers are shown. If you "
        "uncheck the check box, all list view items are collapsed.") );
    chbExpandAll->setAccessibleName ( tr ( "Show all musicians check box" ) );

    // init server address combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    cbxServerAddr->setMaxCount     ( MAX_NUM_SERVER_ADDR_ITEMS );
    cbxServerAddr->setInsertPolicy ( QComboBox::NoInsert );

    // set up list view for connected clients (note that the last column size
    // must not be specified since this column takes all the remaining space)
#ifdef ANDROID
    // for Android we need larger numbers because of the default font size
    lvwServers->setColumnWidth ( 0, 200 );
    lvwServers->setColumnWidth ( 1, 130 );
    lvwServers->setColumnWidth ( 2, 100 );
#else
    lvwServers->setColumnWidth ( 0, 180 );
    lvwServers->setColumnWidth ( 1, 75 );
    lvwServers->setColumnWidth ( 2, 70 );
    lvwServers->setColumnWidth ( 3, 220 );
#endif
    lvwServers->clear();

    // make sure we do not get a too long horizontal scroll bar
    lvwServers->header()->setStretchLastSection ( false );

    // add invisible columns which are used for sorting the list and storing
    // the current/maximum number of clients
    // 0: server name
    // 1: ping time
    // 2: number of musicians (including additional strings like " (full)")
    // 3: location
    // 4: minimum ping time (invisible)
    // 5: maximum number of clients (invisible)
    // 6: number of musicians (just the number, invisible)
    lvwServers->setColumnCount ( 7 );
    lvwServers->hideColumn ( 4 );
    lvwServers->hideColumn ( 5 );
    lvwServers->hideColumn ( 6 );

    // per default the root shall not be decorated (to save space)
    lvwServers->setRootIsDecorated ( false );

    // make sure the connect button has the focus
    butConnect->setFocus();

    // for "show all servers" mode make sort by click on header possible
    if ( bShowCompleteRegList )
    {
        lvwServers->setSortingEnabled ( true );
        lvwServers->sortItems ( 0, Qt::AscendingOrder );
    }

#ifdef ANDROID
    // for the android version maximize the window
    setWindowState ( Qt::WindowMaximized );
#endif


    // Connections -------------------------------------------------------------
    // list view
    QObject::connect ( lvwServers,
        SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( OnServerListItemSelectionChanged() ) );

    QObject::connect ( lvwServers,
        SIGNAL ( itemDoubleClicked ( QTreeWidgetItem*, int ) ),
        this, SLOT ( OnServerListItemDoubleClicked ( QTreeWidgetItem*, int ) ) );

    QObject::connect ( lvwServers, // to get default return key behaviour working
        SIGNAL ( activated ( QModelIndex ) ),
        this, SLOT ( OnConnectClicked() ) );

    // line edit
    QObject::connect ( edtFilter, SIGNAL ( textEdited ( const QString& ) ),
        this, SLOT ( OnFilterTextEdited ( const QString& ) ) );

    // combo boxes
    QObject::connect ( cbxServerAddr, SIGNAL ( editTextChanged ( const QString& ) ),
        this, SLOT ( OnServerAddrEditTextChanged ( const QString& ) ) );

    // check boxes
    QObject::connect ( chbExpandAll, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnExpandAllStateChanged ( int ) ) );

    // buttons
    QObject::connect ( butCancel, SIGNAL ( clicked() ),
        this, SLOT ( close() ) );

    QObject::connect ( butConnect, SIGNAL ( clicked() ),
        this, SLOT ( OnConnectClicked() ) );

    // timers
    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    QObject::connect ( &TimerReRequestServList, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerReRequestServList() ) );
}

void CConnectDlg::Init ( const CVector<QString>& vstrIPAddresses )
{
    // load stored IP addresses in combo box
    cbxServerAddr->clear();
    cbxServerAddr->clearEditText();

    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !vstrIPAddresses[iLEIdx].isEmpty() )
        {
            cbxServerAddr->addItem ( vstrIPAddresses[iLEIdx] );
        }
    }
}

void CConnectDlg::showEvent ( QShowEvent* )
{
    // on opening the connect dialg, we always want to request a
    // new updated server list per definition
    RequestServerList();
}

void CConnectDlg::RequestServerList()
{
    // reset flags
    bServerListReceived      = false;
    bServerListItemWasChosen = false;
    bListFilterWasActive     = false;

    // clear current address and name
    strSelectedAddress    = "";
    strSelectedServerName = "";

    // clear server list view
    lvwServers->clear();

    // clear filter edit box
    edtFilter->setText ( "" );

    // get the IP address of the central server (using the ParseNetworAddress
    // function) when the connect dialog is opened, this seems to be the correct
    // time to do it
    if ( NetworkUtil().ParseNetworkAddress ( strCentralServerAddress,
                                             CentralServerAddress ) )
    {
        // send the request for the server list
        emit ReqServerListQuery ( CentralServerAddress );

        // start timer, if this message did not get any respond to retransmit
        // the server list request message
        TimerReRequestServList.start ( SERV_LIST_REQ_UPDATE_TIME_MS );
    }
}

void CConnectDlg::hideEvent ( QHideEvent* )
{
    // if window is closed, stop timers
    TimerPing.stop();
    TimerReRequestServList.stop();
}

void CConnectDlg::OnTimerReRequestServList()
{
    // if the server list is not yet received, retransmit the request for the
    // server list
    if ( !bServerListReceived )
    {
        // note that this is a connection less message which may get lost
        // and therefore it makes sense to re-transmit it
        emit ReqServerListQuery ( CentralServerAddress );
    }
}

void CConnectDlg::SetServerList ( const CHostAddress&         InetAddr,
                                  const CVector<CServerInfo>& vecServerInfo )
{
    // set flag and disable timer for resend server list request
    bServerListReceived = true;
    TimerReRequestServList.stop();

    // first clear list
    lvwServers->clear();

    // add list item for each server in the server list
    const int iServerInfoLen = vecServerInfo.Size();

    for ( int iIdx = 0; iIdx < iServerInfoLen; iIdx++ )
    {
        // get the host address, note that for the very first entry which is
        // the central server, we have to use the receive host address
        // instead
        CHostAddress CurHostAddress;

        if ( iIdx > 0 )
        {
            CurHostAddress = vecServerInfo[iIdx].HostAddr;
        }
        else
        {
            // substitude the receive host address for central server
            CurHostAddress = InetAddr;
        }

        // create new list view item
        QTreeWidgetItem* pNewListViewItem = new QTreeWidgetItem ( lvwServers );

        // make the entry invisible (will be set to visible on successful ping
        // result) if the complete list of registered servers shall not be shown
        if ( !bShowCompleteRegList )
        {
            pNewListViewItem->setHidden ( true );
        }

        // server name (if empty, show host address instead)
        if ( !vecServerInfo[iIdx].strName.isEmpty() )
        {
            pNewListViewItem->setText ( 0, vecServerInfo[iIdx].strName );
        }
        else
        {
            // IP address and port (use IP number without last byte)
            // Definition: If the port number is the default port number, we do
            // not show it.
            if ( vecServerInfo[iIdx].HostAddr.iPort == LLCON_DEFAULT_PORT_NUMBER )
            {
                // only show IP number, no port number
                pNewListViewItem->setText ( 0, CurHostAddress.toString ( CHostAddress::SM_IP_NO_LAST_BYTE ) );
            }
            else
            {
                // show IP number and port
                pNewListViewItem->setText ( 0, CurHostAddress.toString ( CHostAddress::SM_IP_NO_LAST_BYTE_PORT ) );
            }
        }

        // in case of all servers shown, add the registration number at the beginning
        if ( bShowCompleteRegList )
        {
            pNewListViewItem->setText ( 0, QString ( "%1: " ).arg ( 1 + iIdx, 3 ) + pNewListViewItem->text ( 0 ) );
        }

        // show server name in bold font if it is a permanent server
        QFont CurServerNameFont = pNewListViewItem->font ( 0 );
        CurServerNameFont.setBold ( vecServerInfo[iIdx].bPermanentOnline );
        pNewListViewItem->setFont ( 0, CurServerNameFont );

        // the ping time shall be shown in bold font
        QFont CurPingTimeFont = pNewListViewItem->font ( 1 );
        CurPingTimeFont.setBold ( true );
        pNewListViewItem->setFont ( 1, CurPingTimeFont );

        // server location (city and country)
        QString strLocation = vecServerInfo[iIdx].strCity;

        if ( ( !strLocation.isEmpty() ) &&
             ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry ) )
        {
            strLocation += ", ";
        }

        if ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry )
        {
            QString strCountryToString = QLocale::countryToString ( vecServerInfo[iIdx].eCountry );

            // Qt countryToString does not use spaces in between country name
            // parts but they use upper case letters which we can detect and
            // insert spaces as a post processing
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            if ( !strCountryToString.contains ( " " ) )
            {
                QRegularExpressionMatchIterator reMatchIt = QRegularExpression ( "[A-Z][^A-Z]*" ).globalMatch ( strCountryToString );
                QStringList                     slNames;
                while ( reMatchIt.hasNext() )
                {
                    slNames << reMatchIt.next().capturedTexts();
                }
                strCountryToString = slNames.join ( " " );
            }
#endif

            strLocation += strCountryToString;
        }

        pNewListViewItem->setText ( 3, strLocation );

        // init the minimum ping time with a large number (note that this number
        // must fit in an integer type)
        pNewListViewItem->setText ( 4, "99999999" );

        // store the maximum number of clients
        pNewListViewItem->setText ( 5, QString().setNum ( vecServerInfo[iIdx].iMaxNumClients ) );

        // initialize the current number of connected clients
        pNewListViewItem->setText ( 6, QString().setNum ( 0 ) );

        // store host address
        pNewListViewItem->setData ( 0, Qt::UserRole, CurHostAddress.toString() );

        // per default expand the list item (if not "show all servers")
        if ( bShowAllMusicians )
        {
            lvwServers->expandItem ( pNewListViewItem );
        }
    }

    // immediately issue the ping measurements and start the ping timer since
    // the server list is filled now
    OnTimerPing();
    TimerPing.start ( PING_UPDATE_TIME_SERVER_LIST_MS );
}

void CConnectDlg::SetConnClientsList ( const CHostAddress&          InetAddr,
                                       const CVector<CChannelInfo>& vecChanInfo )
{
    // find the server with the correct address
    QTreeWidgetItem* pCurListViewItem = FindListViewItem ( InetAddr );

    if ( pCurListViewItem )
    {
        // first remove any existing childs
        DeleteAllListViewItemChilds ( pCurListViewItem );

        // get number of connected clients
        const int iNumConnectedClients = vecChanInfo.Size();

        for ( int i = 0; i < iNumConnectedClients; i++ )
        {
            // create new list view item
            QTreeWidgetItem* pNewChildListViewItem =
                new QTreeWidgetItem ( pCurListViewItem );

            // child items shall use only one column
            pNewChildListViewItem->setFirstColumnSpanned ( true );

            // set the clients name
            QString sClientText = vecChanInfo[i].GenNameForDisplay();

            // set the icon: country flag has priority over instrument
            bool bCountryFlagIsUsed = false;

            if ( vecChanInfo[i].eCountry != QLocale::AnyCountry )
            {
                // try to load the country flag icon
                QPixmap CountryFlagPixmap (
                    CLocale::GetCountryFlagIconsResourceReference ( vecChanInfo[i].eCountry ) );

                // first check if resource reference was valid
                if ( !CountryFlagPixmap.isNull() )
                {
                    // set correct picture
                    pNewChildListViewItem->setIcon ( 0, QIcon ( CountryFlagPixmap ) );

                    // add the instrument information as text
                    if ( !CInstPictures::IsNotUsedInstrument ( vecChanInfo[i].iInstrument ) )
                    {
                        sClientText.append ( " (" +
                            CInstPictures::GetName ( vecChanInfo[i].iInstrument ) + ")" );
                    }

                    bCountryFlagIsUsed = true;
                }
            }

            if ( !bCountryFlagIsUsed )
            {
                // get the resource reference string for this instrument
                const QString strCurResourceRef =
                    CInstPictures::GetResourceReference ( vecChanInfo[i].iInstrument );

                // first check if instrument picture is used or not and if it is valid
                if ( !( CInstPictures::IsNotUsedInstrument ( vecChanInfo[i].iInstrument ) ||
                        strCurResourceRef.isEmpty() ) )
                {
                    // set correct picture
                    pNewChildListViewItem->setIcon ( 0, QIcon ( QPixmap ( strCurResourceRef ) ) );
                }
            }

            // apply the client text to the list view item
            pNewChildListViewItem->setText ( 0, sClientText );

            // add the new child to the corresponding server item
            pCurListViewItem->addChild ( pNewChildListViewItem );

            // at least one server has childs now, show decoration to be able
            // to show the childs
            lvwServers->setRootIsDecorated ( true );
        }
    }
}

void CConnectDlg::OnServerListItemSelectionChanged()
{
    // get current selected item (we are only interested in the first selcted
    // item)
    QList<QTreeWidgetItem*> CurSelListItemList = lvwServers->selectedItems();

    // if an item is clicked/selected, copy the server name to the combo box
    if ( CurSelListItemList.count() > 0 )
    {
        // make sure no signals are send when we change the text
        cbxServerAddr->blockSignals ( true );
        {
            cbxServerAddr->setEditText (
                GetParentListViewItem ( CurSelListItemList[0] )->text ( 0 ) );
        }
        cbxServerAddr->blockSignals ( false );
    }
}

void CConnectDlg::OnServerListItemDoubleClicked ( QTreeWidgetItem* Item,
                                                  int )
{
    // if a server list item was double clicked, it is the same as if the
    // connect button was clicked
    if ( Item != nullptr )
    {
        OnConnectClicked();
    }
}

void CConnectDlg::OnServerAddrEditTextChanged ( const QString& )
{
    // in the server address combo box, a text was changed, remove selection
    // in the server list (if any)
    lvwServers->clearSelection();
}

void CConnectDlg::ShowAllMusicians ( const bool bState )
{
    bShowAllMusicians = bState;

    // update list
    if ( bState )
    {
        lvwServers->expandAll();
    }
    else
    {
        lvwServers->collapseAll();
    }

    // update check box if necessary
    if ( ( chbExpandAll->checkState() == Qt::Checked && !bShowAllMusicians ) ||
         ( chbExpandAll->checkState() == Qt::Unchecked && bShowAllMusicians ) )
    {
        chbExpandAll->setCheckState ( bState ? Qt::Checked : Qt::Unchecked );
    }
}

void CConnectDlg::UpdateListFilter()
{
    const QString sFilterText = edtFilter->text();

    if ( !sFilterText.isEmpty() )
    {
        bListFilterWasActive     = true;
        const int iServerListLen = lvwServers->topLevelItemCount();

        for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
        {
            QTreeWidgetItem* pCurListViewItem = lvwServers->topLevelItem ( iIdx );
            bool             bFilterFound     = false;

            // search server name
            if ( pCurListViewItem->text ( 0 ).indexOf ( sFilterText, 0, Qt::CaseInsensitive ) >= 0 )
            {
                bFilterFound = true;
            }

            // search location
            if ( pCurListViewItem->text ( 3 ).indexOf ( sFilterText, 0, Qt::CaseInsensitive ) >= 0 )
            {
                bFilterFound = true;
            }

            // search childs
            for ( int iCCnt = 0; iCCnt < pCurListViewItem->childCount(); iCCnt++ )
            {
                if ( pCurListViewItem->child ( iCCnt )->text ( 0 ).indexOf ( sFilterText, 0, Qt::CaseInsensitive ) >= 0 )
                {
                    bFilterFound = true;
                }
            }

            // only update Hide state if ping time was received
            if ( !pCurListViewItem->text ( 1 ).isEmpty() || bShowCompleteRegList )
            {
                // only update hide and expand status if the hide state has to be changed to
                // preserve if user clicked on expand icon manually
                if ( ( pCurListViewItem->isHidden() && bFilterFound ) ||
                     ( !pCurListViewItem->isHidden() && !bFilterFound ) )
                {
                    pCurListViewItem->setHidden ( !bFilterFound );
                    pCurListViewItem->setExpanded ( bShowAllMusicians );
                }
            }
        }
    }
    else
    {
        // if the filter was active but is now disabled, we have to update all list
        // view items for the "ping received" hide state
        if ( bListFilterWasActive )
        {
            const int iServerListLen = lvwServers->topLevelItemCount();

            for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
            {
                QTreeWidgetItem* pCurListViewItem = lvwServers->topLevelItem ( iIdx );

                // if ping time is empty, hide item (if not already hidden)
                if ( pCurListViewItem->text ( 1 ).isEmpty() && !bShowCompleteRegList )
                {
                    pCurListViewItem->setHidden ( true );
                }
                else
                {
                    // in case it was hidden, show it and take care of expand
                    if ( pCurListViewItem->isHidden() )
                    {
                        pCurListViewItem->setHidden ( false );
                        pCurListViewItem->setExpanded ( bShowAllMusicians );
                    }
                }
            }

            bListFilterWasActive = false;
        }
    }
}

void CConnectDlg::OnConnectClicked()
{
    // get the IP address to be used according to the following definitions:
    // - if the list has focus and a line is selected, use this line
    // - if the list has no focus, use the current combo box text
    QList<QTreeWidgetItem*> CurSelListItemList = lvwServers->selectedItems();

    if ( CurSelListItemList.count() > 0 )
    {
        // get the parent list view item
        QTreeWidgetItem* pCurSelTopListItem =
            GetParentListViewItem ( CurSelListItemList[0] );

        // get host address from selected list view item as a string
        strSelectedAddress =
            pCurSelTopListItem->data ( 0, Qt::UserRole ).toString();

        // store selected server name
        strSelectedServerName = pCurSelTopListItem->text ( 0 );

        // set flag that a server list item was chosen to connect
        bServerListItemWasChosen = true;
    }
    else
    {
        strSelectedAddress = cbxServerAddr->currentText();
    }

    // tell the parent window that the connection shall be initiated
    done ( QDialog::Accepted );
}

void CConnectDlg::OnTimerPing()
{
    // send ping messages to the servers in the list
    const int iServerListLen = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        CHostAddress CurServerAddress;

        // try to parse host address string which is stored as user data
        // in the server list item GUI control element
        if ( NetworkUtil().ParseNetworkAddress (
                lvwServers->topLevelItem ( iIdx )->
                data ( 0, Qt::UserRole ).toString(),
                CurServerAddress ) )
        {
            // if address is valid, send ping or the version and OS request
#ifdef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING
            emit CreateCLServerListReqVerAndOSMes ( CurServerAddress );
#else
            emit CreateCLServerListPingMes ( CurServerAddress );
#endif

            // check if the number of child list items matches the number of
            // connected clients, if not then request the client names
            if ( lvwServers->topLevelItem ( iIdx )->text ( 6 ).toInt() !=
                 lvwServers->topLevelItem ( iIdx )->childCount() )
            {
                emit CreateCLServerListReqConnClientsListMes ( CurServerAddress );
            }
        }
    }
}

void CConnectDlg::SetPingTimeAndNumClientsResult ( const CHostAddress& InetAddr,
                                                   const int           iPingTime,
                                                   const int           iNumClients )
{
    // apply the received ping time to the correct server list entry
    QTreeWidgetItem* pCurListViewItem = FindListViewItem ( InetAddr );

    if ( pCurListViewItem )
    {
        // check if this is the first time a ping time is set
        const bool bIsFirstPing = pCurListViewItem->text ( 1 ).isEmpty();
        bool       bDoSorting   = false;

        // update minimum ping time column (invisible, used for sorting) if
        // the new value is smaller than the old value
        int iMinPingTime = pCurListViewItem->text ( 4 ).toInt();

        if ( iMinPingTime > iPingTime )
        {
            // update the minimum ping time with the new lowest value
            iMinPingTime = iPingTime;

            // we pad to a total of 8 characters with zeros to make sure the
            // sorting is done correctly
            pCurListViewItem->setText ( 4, QString ( "%1" ).arg (
                iPingTime, 8, 10, QLatin1Char ( '0' ) ) );

            // update the sorting (lowest number on top)
            bDoSorting = true;
        }

        // for debugging it is good to see the current ping time in the list
        // and not the minimum ping time -> overwrite the value for debugging
        if ( bShowCompleteRegList )
        {
            iMinPingTime = iPingTime;
        }

        // Only show minimum ping time in the list since this is the important
        // value. Temporary bad ping measurements are of no interest.
        // Color definition: <= 25 ms green, <= 50 ms yellow, otherwise red
        if ( iMinPingTime <= 25 )
        {
            pCurListViewItem->setForeground ( 1, Qt::darkGreen );
        }
        else
        {
            if ( iMinPingTime <= 50 )
            {
                pCurListViewItem->setForeground ( 1, Qt::darkYellow );
            }
            else
            {
                pCurListViewItem->setForeground ( 1, Qt::red );
            }
        }

        // update ping text, take special care if ping time exceeds a
        // certain value
        if ( iMinPingTime > 500 )
        {
            pCurListViewItem->setText ( 1, ">500 ms" );
        }
        else
        {
            pCurListViewItem->
                setText ( 1, QString().setNum ( iMinPingTime ) + " ms" );
        }

        // update number of clients text
        if ( iNumClients >= pCurListViewItem->text ( 5 ).toInt() )
        {
            pCurListViewItem->
                setText ( 2, QString().setNum ( iNumClients ) + " (full)" );
        }
        else
        {
            pCurListViewItem->
                setText ( 2, QString().setNum ( iNumClients ) );
        }

        // update number of clients value (hidden)
        pCurListViewItem->setText ( 6, QString().setNum ( iNumClients ) );

        // this is the first time a ping time was received, set item to visible
        if ( bIsFirstPing )
        {
            pCurListViewItem->setHidden ( false );
        }

        // Update sorting. Note that the sorting must be the last action for the
        // current item since the topLevelItem ( iIdx ) is then no longer valid.
        if ( bDoSorting && !bShowCompleteRegList ) // do not sort if "show all servers"
        {
            lvwServers->sortByColumn ( 4, Qt::AscendingOrder );
        }
    }

    // if no server item has childs, do not show decoration
    bool bAnyListItemHasChilds = false;
    const int iServerListLen   = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        // check if the current list item has childs
        if ( lvwServers->topLevelItem ( iIdx )->childCount() > 0 )
        {
            bAnyListItemHasChilds = true;
        }
    }

    if ( !bAnyListItemHasChilds )
    {
        lvwServers->setRootIsDecorated ( false );
    }

    // we may have changed the Hidden state for some items, if a filter was active, we now
    // have to update it to void lines appear which do not satisfy the filter criteria
    UpdateListFilter();
}

QTreeWidgetItem* CConnectDlg::FindListViewItem ( const CHostAddress& InetAddr )
{
    const int iServerListLen = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        // compare the received address with the user data string of the
        // host address by a string compare
        if ( !lvwServers->topLevelItem ( iIdx )->
                data ( 0, Qt::UserRole ).toString().
                compare ( InetAddr.toString() ) )
        {
            return lvwServers->topLevelItem ( iIdx );
        }
    }

    return nullptr;
}

QTreeWidgetItem* CConnectDlg::GetParentListViewItem ( QTreeWidgetItem* pItem )
{
    // check if the current item is already the top item, i.e. the parent
    // query fails and returns null
    if ( pItem->parent() )
    {
        // we only have maximum one level, i.e. if we call the parent function
        // we are at the top item
        return pItem->parent();
    }
    else
    {
        // this item is already the top item
        return pItem;
    }
}

void CConnectDlg::DeleteAllListViewItemChilds ( QTreeWidgetItem* pItem )
{
    // loop over all childs
    while ( pItem->childCount() > 0 )
    {
        // get the first child in the list
        QTreeWidgetItem* pCurChildItem = pItem->child ( 0 );

        // remove it from the item (note that the object is not deleted)
        pItem->removeChild ( pCurChildItem );

        // delete the object to avoid a memory leak
        delete pCurChildItem;
    }
}

#ifdef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING
void CConnectDlg::SetVersionAndOSType ( CHostAddress           InetAddr,
                                        COSUtil::EOpSystemType eOSType,
                                        QString                strVersion )
{
    // apply the received version and OS type to the correct server list entry
    QTreeWidgetItem* pCurListViewItem = FindListViewItem ( InetAddr );

    if ( pCurListViewItem )
    {
// TEST since this is just a debug info, we just reuse the ping column (note
// the we have to replace the ping message emit with the version and OS request
// so that this works, see above code)
pCurListViewItem->
    setText ( 1, strVersion + "/" + COSUtil::GetOperatingSystemString ( eOSType ) );

// a version and OS type was received, set item to visible
if ( pCurListViewItem->isHidden() )
{
    pCurListViewItem->setHidden ( false );
}
    }
}
#endif
