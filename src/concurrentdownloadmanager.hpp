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

#ifndef CCDOWNLOAD_MG_H
#define CCDOWNLOAD_MG_H

#include <QTableWidget>
#include <QStringList>
#include <QPushButton>

#include <memory>

#include "engines.h"
#include "context.hpp"

#include "utility.h"

struct concurrentDownloadManagerFinishedStatus
{
	const int index ;
	const bool cancelled ;
	const bool allFinished ;
	const bool finishedSuccess ;

	static QString notStarted()
	{
		return "Not Started" ;
	}
	static QString running()
	{
		return "Running" ;
	}
	static QString finishedCancelled()
	{
		return "FinishedCancelled" ;
	}
	static QString finishedWithError()
	{
		return "FinishedWithError" ;
	}
	static QString finishedWithSuccess()
	{
		return "FinishedWithSuccess" ;
	}
	static bool notStarted( const QString& e )
	{
		return e == "Not Started" ;
	}
	static bool running( const QString& e )
	{
		return running() == e ;
	}
	static bool finishedCancelled( const QString& e )
	{
		return finishedCancelled() == e ;
	}
	static bool finishedWithError( const QString& e )
	{
		return finishedWithError() == e ;
	}
	static bool finishedWithSuccess( const QString& e )
	{
		return finishedWithSuccess() == e  ;
	}
	void setState( QTableWidgetItem& item ) const
	{
		if( this->cancelled ){

			item.setText( finishedCancelled() ) ;

		}else if( this->finishedSuccess ){

			item.setText( finishedWithSuccess() ) ;
		}else{
			item.setText( finishedWithError() ) ;
		}
	}
};

template< typename Index,
	  typename EnableAll >
class concurrentDownloadManager
{
public:
	concurrentDownloadManager( const Context& ctx,
				   Index index,
				   QLineEdit& lineEdit,
				   QPushButton& cancelButton ) :
		m_index( std::move( index ) ),
		m_enableAll( ctx ),
		m_ctx( ctx ),
		m_lineEdit( lineEdit ),
		m_table( m_index.table() ),
		m_cancelButton( cancelButton )
	{
	}
	void cancelled()
	{
		m_cancelled = true ;
	}
	template< typename Function,typename Finished >
	void monitorForFinished( const engines::engine& engine,
				 int index,
				 bool success,
				 Function function,
				 Finished finished )
	{
		if( m_cancelled ){

			this->uiEnableAll( true ) ;
			m_cancelButton.setEnabled( false ) ;

			finished( concurrentDownloadManagerFinishedStatus{ index,true,false,false } ) ;
		}else{
			m_counter++ ;

			if( m_counter == m_index.count() ){

				this->uiEnableAll( true ) ;
				m_cancelButton.setEnabled( false ) ;

				finished( concurrentDownloadManagerFinishedStatus{ index,false,true,success } ) ;
			}else{
				finished( concurrentDownloadManagerFinishedStatus{ index,false,false,success } ) ;

				if( m_index.hasNext() ){

					function( engine,m_index.value() ) ;
				}
			}
		}
	}
	template< typename ConcurrentDownload >
	void download( const engines::engine& engine,
		       int maxNumberOfConcurrency,
		       ConcurrentDownload concurrentDownload )
	{
		if( m_table.rowCount() ){

			m_counter = 0 ;
			m_cancelled = false ;
			m_index.reset() ;

			this->uiEnableAll( false ) ;
			m_cancelButton.setEnabled( true ) ;
			m_table.setEnabled( true ) ;			

			auto max = [ & ](){

				auto count = m_index.count() ;

				if( maxNumberOfConcurrency < count ){

					return maxNumberOfConcurrency ;
				}else{
					return count ;
				}
			}() ;

			for( int s = 0 ; s < max ; s++ ){

				concurrentDownload( engine,m_index.value( s ) ) ;
			}
		}
	}
	template< typename Options,typename Logger >
	void download( const engines::engine& engine,
		       int index,
		       const QString& url,
		       Options opts,
		       Logger logger )
	{
		Q_UNUSED( index )

		m_index++ ;

		auto m = m_lineEdit.text() ;

		utility::args args( m_lineEdit.text() ) ;

		auto u = url ;

		if( !u.isEmpty() ){

			u = utility::split( u,'\n',true ).at( 0 ) ;
		}

		utility::run( engine,
			      utility::updateOptions( engine,args,{ u } ),
			      args.quality,
			      std::move( opts ),
			      std::move( logger ),
			      utility::make_term_conn( &m_cancelButton,&QPushButton::clicked ) ) ;
	}
private:
	void uiEnableAll( bool e )
	{
		m_enableAll( e ) ;
	}
	int m_counter ;
	Index m_index ;
	EnableAll m_enableAll ;
	bool m_cancelled ;
	const Context& m_ctx ;
	QLineEdit& m_lineEdit ;
	QTableWidget& m_table ;
	QPushButton& m_cancelButton ;
} ;

#endif
