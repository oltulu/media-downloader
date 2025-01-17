/*
 *
 *  Copyright (c) 2021
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "batchdownloader.h"
#include "tabmanager.h"

batchdownloader::batchdownloader( const Context& ctx ) :
	m_ctx( ctx ),
	m_settings( m_ctx.Settings() ),
	m_ui( m_ctx.Ui() ),
	m_mainWindow( m_ctx.mainWidget() ),
	m_tabManager( m_ctx.TabManager() ),
	m_running( false ),
	m_debug( ctx.debug() ),
	m_ccmd( m_ctx,
		batchdownloader::Index( m_downloadEntries,*m_ui.tableWidgetBD ),
		*m_ui.lineEditBDUrlOptions,
		*m_ui.pbBDCancel )
{
	m_ui.tabWidgetBatchDownlader->setCurrentIndex( 0 ) ;

	utility::setTableWidget( *m_ui.tableWidgetBD ) ;

	m_ui.tableWidgetBD->hideColumn( 1 ) ;
	m_ui.tableWidgetBD->hideColumn( 2 ) ;

	m_ui.pbBDDownload->setEnabled( false ) ;

	m_ui.pbBDCancel->setEnabled( false ) ;

	this->resetMenu() ;

	connect( m_ui.tabWidgetBatchDownlader,&QTabWidget::currentChanged,[ this ]( int s ){

		if( s == 0 ){

			m_ui.pbBDDownload->setEnabled( m_ui.tableWidgetBD->rowCount() ) ;
		}
	} ) ;

	connect( m_ui.tableWidgetBD,&QTableWidget::customContextMenuRequested,[ this ]( QPoint ){

		if( m_running ){


		}else{
			if( m_ui.tableWidgetBD->rowCount() > 0 ){

				QMenu m ;

				connect( m.addAction( tr( "Remove" ) ),&QAction::triggered,[ this ](){

					auto row = m_ui.tableWidgetBD->currentRow() ;

					if( row != -1 ){

						m_ui.tableWidgetBD->removeRow( row ) ;

						m_ui.pbBDDownload->setEnabled( m_ui.tableWidgetBD->rowCount() ) ;
					}
				} ) ;

				m.exec( QCursor::pos() ) ;
			}
		}
	} ) ;

	connect( m_ui.pbBDQuit,&QPushButton::clicked,[ this ](){

		m_tabManager.basicDownloader().appQuit() ;
	} ) ;

	connect( m_ui.pbBDCancel,&QPushButton::clicked,[ this ](){

		m_ccmd.cancelled() ;
	} ) ;

	connect( m_ui.pbBDAdd,&QPushButton::clicked,[ this ](){

		this->addToList( m_ui.lineEditBDUrl->text(),m_settings.doNotGetUrlTitle() ) ;
	} ) ;

	connect( m_ui.pbBDDownload,&QPushButton::clicked,[ this ](){

		this->download( m_ctx.Engines().defaultEngine() ) ;
	} ) ;
}

void batchdownloader::init_done()
{
}

void batchdownloader::resetMenu()
{
	utility::setMenuOptions( m_ctx,{},true,m_ui.pbBDOptions,[ this ]( QAction * aa ){

		utility::selectedAction ac( aa ) ;

		if( ac.clearOptions() ){

			m_ui.lineEditBDUrlOptions->clear() ;

		}else if( ac.clearScreen() ){

			this->clearScreen() ;

		}else if( ac.openFolderPath() ){

			utility::openDownloadFolderPath( m_settings.downloadFolder() ) ;
		}else{
			m_ui.lineEditBDUrlOptions->setText( ac.objectName() ) ;

			this->download( m_ctx.Engines().defaultEngine() ) ;
		}
	} ) ;
}

void batchdownloader::retranslateUi()
{
	this->resetMenu() ;
}

void batchdownloader::tabEntered()
{
}

void batchdownloader::tabExited()
{
}

void batchdownloader::download( const engines::engine& engine,
				const QString& opts,
				const QStringList& list,
				bool doNotGetTitle )
{
	//utility::clear( *m_ui.tableWidgetBD ) ;

	for( const auto& it : list ){

		this->addToList( it,doNotGetTitle ) ;
	}

	m_ui.tabWidget->setCurrentIndex( 1 ) ;

	m_ui.tabWidgetBatchDownlader->setCurrentIndex( 0 ) ;

	m_ui.lineEditBDUrlOptions->setText( opts ) ;

	this->download( engine ) ;
}

void batchdownloader::clearScreen()
{
	utility::clear( *m_ui.tableWidgetBD ) ;
	m_ui.lineEditBDUrlOptions->clear() ;
	m_ui.lineEditBDUrl->clear() ;
}

static void _set_variables( Ui::MainWindow& ui,const QString& url,const QString& state,QWidget& widget )
{
	utility::addItem( *ui.tableWidgetBD,{ url,url,state },widget.font() ) ;

	ui.lineEditBDUrl->clear() ;

	ui.lineEditBDUrl->setFocus() ;

	ui.pbBDDownload->setEnabled( true ) ;
}

template< typename Function >
static void _getUrlTitle( const QString& exe,const QStringList& args,Function function )
{
	utility::run( exe,args,[]( QProcess& exe ){

		exe.setProcessChannelMode( QProcess::ProcessChannelMode::SeparateChannels ) ;

		return QString() ;

	},[]( QProcess& ){},[ function = std::move( function ) ]( int s,QProcess::ExitStatus e,QString& c ){

			if( s == 0 && e == QProcess::ExitStatus::NormalExit ){

				function( c ) ;
			}else{
				function( QString() ) ;
			}

	},[]( QProcess::ProcessChannel channel,QByteArray data,QString& c ){

		if( channel == QProcess::ProcessChannel::StandardOutput ){

			c = data ;
		}
	} ) ;
}

void batchdownloader::addToList( const QString& a,bool doNotGetTitle )
{
	if( !a.isEmpty() ){

		const auto& engine = m_ctx.Engines().defaultEngine() ;

		if( doNotGetTitle || !engine.likeYoutubeDl() ){

			auto s = concurrentDownloadManagerFinishedStatus::notStarted() ;

			_set_variables( m_ui,a,s,m_mainWindow ) ;
		}else{
			m_ctx.TabManager().disableAll() ;

			const auto& exe = engine.exePath().realExe() ;
			QStringList args{ "--get-title",a } ;

			engines::engine::exeArgs::cmd cmd( exe,args ) ;

			m_ctx.logger().add( "cmd: " + engine.commandString( cmd ) ) ;

			_getUrlTitle( exe,args,[ a,this ]( const QString& title ){

				auto state = concurrentDownloadManagerFinishedStatus::notStarted() ;

				if( title.isEmpty() || title == "\n" ){

					_set_variables( m_ui,a,state,m_mainWindow ) ;
				}else{
					m_ctx.logger().add( title ) ;
					_set_variables( m_ui,a + "\n" + title,state,m_mainWindow ) ;
				}

				m_ctx.TabManager().enableAll() ;
			} ) ;
		}
	}
}

void batchdownloader::download( const engines::engine& engine )
{
	this->addToList( m_ui.lineEditBDUrl->text(),true ) ;

	m_downloadEntries.clear() ;

	for( int s = 0 ; s < m_ui.tableWidgetBD->rowCount() ; s++ ){

		auto e = m_ui.tableWidgetBD->item( s,2 )->text() ;

		if( !concurrentDownloadManagerFinishedStatus::finishedWithSuccess( e ) ){

			m_downloadEntries.emplace_back( s ) ;
		}
	}

	if( m_downloadEntries.empty() ){

		return ;
	}

	m_ccmd.download( engine,[ this ](){

		if( m_settings.concurrentDownloading() ){

			return m_settings.maxConcurrentDownloads() ;
		}else{
			return 1 ;
		}

	}(),[ this ]( const engines::engine& engine,int index ){

		this->download( engine,index ) ;
	} ) ;
}

void batchdownloader::download( const engines::engine& engine,int index )
{
	auto aa = batchdownloader::make_options( *m_ui.pbBDCancel,m_ctx,m_debug,[ &engine,index,this ]( bool e ){

		m_ccmd.monitorForFinished( engine,index,e,[ this ]( const engines::engine& engine,int index ){

			this->download( engine,index ) ;

		},[ &engine,this ]( const concurrentDownloadManagerFinishedStatus& f ){

			utility::updateFinishedState( engine,m_settings,*m_ui.tableWidgetBD,f ) ;
		} ) ;
	} ) ;

	m_ccmd.download( engine,
			 index,
			 m_ui.tableWidgetBD->item( index,1 )->text(),
			 std::move( aa ),
			 make_loggerBatchDownloader( engine.filter(),
						     engine,
						     m_ctx.logger(),
						     *m_ui.tableWidgetBD->item( index,0 ),
						     utility::concurrentID() ) ) ;
}

void batchdownloader::enableAll()
{
	m_running = false ;

	m_ui.tableWidgetBD->setEnabled( true ) ;
	m_ui.pbBDDownload->setEnabled( m_ui.tableWidgetBD->rowCount() ) ;
	m_ui.pbBDAdd->setEnabled( true ) ;
	m_ui.pbBDOptions->setEnabled( true ) ;
	m_ui.labelBDEnterOptions->setEnabled( true ) ;
	m_ui.labelBDEnterUrl->setEnabled( true ) ;
	m_ui.pbBDQuit->setEnabled( true ) ;
	m_ui.lineEditBDUrl->setEnabled( true ) ;
	m_ui.lineEditBDUrlOptions->setEnabled( true ) ;
	m_ui.pbBDCancel->setEnabled( true ) ;
}

void batchdownloader::disableAll()
{
	m_running = true ;

	m_ui.pbBDCancel->setEnabled( false ) ;
	m_ui.tableWidgetBD->setEnabled( false ) ;
	m_ui.pbBDDownload->setEnabled( false ) ;
	m_ui.pbBDAdd->setEnabled( false ) ;
	m_ui.pbBDOptions->setEnabled( false ) ;
	m_ui.labelBDEnterOptions->setEnabled( false ) ;
	m_ui.labelBDEnterUrl->setEnabled( false ) ;
	m_ui.pbBDQuit->setEnabled( false ) ;
	m_ui.lineEditBDUrl->setEnabled( false ) ;
	m_ui.lineEditBDUrlOptions->setEnabled( false ) ;
}

void batchdownloader::EnableAll::operator()( bool e )
{
	if( e ){

		m_tabManager.enableAll() ;
	}else{
		m_tabManager.disableAll() ;
	}
}
