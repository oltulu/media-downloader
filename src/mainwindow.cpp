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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"
#include "tabmanager.h"

#include "context.hpp"
#include "settings.h"
#include "translator.h"

static std::unique_ptr< Ui::MainWindow > _init_ui( QMainWindow& mw )
{
	auto m = std::make_unique< Ui::MainWindow >() ;
	m->setupUi( &mw ) ;
	return m ;
}

MainWindow::MainWindow( QApplication& app,settings& s,translator& t ) :
	m_qApp( app ),
	m_ui( _init_ui( *this ) ),
	m_showTrayIcon( s.showTrayIcon() ),
	m_logger( *m_ui->plainTextEditLogger ),
	m_engines( m_logger,s ),
	m_tabManager( s,t,m_engines,m_logger,*m_ui,*this,*this ),
	m_settings( s )
{
	this->window()->setFixedSize( this->window()->size() ) ;

	QIcon icon = QIcon::fromTheme( "media-downloader",QIcon( ":media-downloader" ) ) ;

	this->window()->setWindowIcon( icon ) ;

	if( m_showTrayIcon ){

		m_trayIcon.setIcon( icon ) ;

		m_trayIcon.setContextMenu( [ this,&t ](){

			auto m = new QMenu( this ) ;

			auto ac = t.addAction( m,{ tr( "Quit" ),"Quit","Quit" },true ) ;

			connect( ac,&QAction::triggered,[ this ](){

				m_tabManager.basicDownloader().appQuit() ;
			} ) ;

			return m ;
		}() ) ;

		connect( &m_trayIcon,&QSystemTrayIcon::activated,[ this ]( QSystemTrayIcon::ActivationReason ){

			if( this->isVisible() ){

				this->hide() ;
			}else{
				this->show() ;
			}
		} ) ;

		if( QSystemTrayIcon::isSystemTrayAvailable() ){

			m_trayIcon.show() ;
		}else{
			utility::Timer( 1000,[ this ]( int counter ){

				if( QSystemTrayIcon::isSystemTrayAvailable() ){

					m_trayIcon.show() ;

					return true ;
				}else{
					if( counter == 5 ){

						/*
						 * We have waited for system tray to become
						 * available and we can wait no longer, display
						 * it and hope for the best.
						 */
						m_trayIcon.show() ;

						return true ;
					}else{
						return false ;
					}
				}

			} ) ;
		}

		m_trayIcon.show() ;
	}
}

void MainWindow::retranslateUi()
{
	m_ui->retranslateUi( this ) ;
}

int MainWindow::exec()
{
	this->show() ;
	return m_qApp.exec() ;
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent( QCloseEvent * e )
{
	e->ignore() ;

	this->hide() ;

	if( !m_showTrayIcon ){

		m_tabManager.basicDownloader().appQuit() ;
	}
}
