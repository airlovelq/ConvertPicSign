// (C) Copyright 2002-2012 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.cpp
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "opencv.hpp"
#include <vector>
using namespace std;
#include "PubFunction.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000
//-----------------------------------------------------------------------------
#define szRDS _RXST("")



void Convert()
{
	CString strFileName(_T(""));
	CFileDialog dlg(TRUE, _T(".jpg"), _T("*.jpg"),OFN_OVERWRITEPROMPT, _T("image files (*.jpeg; *.jpg; *.bmp) |*.jpeg; *.jpg; *.bmp | All Files (*.*) |*.*||"), ::acedGetAcadFrame());
	if(dlg.DoModal() == IDOK)
		strFileName = dlg.GetPathName();
	else
		return;

	ads_name sname;                     //最终的轮廓选择集
	acedSSAdd( NULL , NULL , sname ) ;

	char *imagename = arxPub::ChangeCstringToCh(strFileName);
	//const char *imagename = "f:\\签名.jpg";
	//cv::Mat imgpre = cv::imread(imagename);
	cv::Mat img = cv::imread(imagename);        //原图
	cv::Mat imggray;                            //灰度图
	cv::cvtColor(img,imggray,CV_BGR2GRAY);      //转换为灰度图
	
	
	//cv::adaptiveThreshold(imggray,imggray, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 3, 3);   //自适应阈值处理
	//cv::threshold(edges,edges,130,255,4);
	//cv::medianBlur(imggray,imggray,3);
	//cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));  
    //dilate(imggray,imggray,element);  
	cv::blur(imggray,imggray,cv::Size(5,5));       //降噪
	cv::Canny(imggray,imggray,10,50,3);            //canny算子检测轮廓
	//cv::boxFilter(imggray,imggray,-1,cv::Size(5,5));
	//cv::blur(edges,edges,cv::Size(3,3));
	//cv::bitwise_not(imggray,imggray);

	//寻找边缘
	vector<vector<cv::Point>> conts;
	vector<cv::Vec4i> his;
	cv::findContours(imggray,conts,his,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE,cv::Point(0,0));
	//cv::Mat draw = cv::Mat::zeros(imggray.size(),CV_8UC3);
	//AcDbPolyline *pPoly = new AcDbPolyline();
	int n=1;

	//将所有边缘点置于一个vector中
	vector<cv::Point> ptall;
	for ( int i = 0; i<conts.size() ; i++ )
	{
		//cv::drawContours(draw,conts,i,cv::Scalar(60,60,60),cv::FILLED,8,his,0,cv::Point());
		for ( int j=0 ; j<conts.at(i).size() ; j++ )
		{
			ptall.push_back(conts.at(i).at(j));
			//AcGePoint2d pt(conts.at(i).at(j).x, conts.at(i).at(j).y);
			//pPoly->addVertexAt(n,pt);
			//n++;
		}
	}

	//cv::Point ptlast = ptall.at(0);
	//寻找最近的点作为下一点，在cad中绘制多段线
	cv::Point pt = ptall.at(0);
	cv::Point ptnext = pt;
	ptall.erase(ptall.begin());
	int size = ptall.size();
	AcDbPolyline *pPoly = new AcDbPolyline();
	AcGePoint2d pt2d(pt.x,-pt.y);
	pPoly->addVertexAt(0,pt2d);
	for ( int i=0 ; i<size ; i++ )
	{
		double distance = 100000.0;
		for ( int j=0 ; j<ptall.size() ; j++ )
		{
			if (sqrt(pow(double(pt.x-ptall.at(j).x),2)+pow(double(pt.y-ptall.at(j).y),2))<distance) //&& (ptlast!=ptall.at(j)) && (pt!=ptall.at(j)))
			{
				distance = sqrt(pow(double(pt.x-ptall.at(j).x),2)+pow(double(pt.y-ptall.at(j).y),2));
				ptnext = ptall.at(j);
			}
		}
		if ( distance<10  )                    //如果下一个点距离过大，说明是下一个文字了，应该断开，闭合之前文字的多段线，这个值没有很好的办法去自适应，目前定为10
		{
			AcGePoint2d pt2dn(ptnext.x,-ptnext.y);
			pPoly->addVertexAt(n,pt2dn);
			n++;
		//AcGePoint3d ptfirst(pt.x,pt.y,0);
	    //AcGePoint3d ptSecond(ptnext.x,ptnext.y,0);
		//AcDbLine *pLine = new AcDbLine(ptfirst,ptSecond);
		//PostToModelSpace(pLine);
		}
		else
		{
			pPoly->setClosed(Adesk::kTrue);
			arxPub::PostToModelSpace(pPoly);
			ads_name single;
			acdbGetAdsName(single,pPoly->objectId());
			acedSSAdd(single,sname,sname);                //将多段线放入选择集
			pPoly=new AcDbPolyline();
			n=0;
			AcGePoint2d pt2dn(ptnext.x,-ptnext.y);
			pPoly->addVertexAt(n,pt2dn);
		}
		//ptlast = pt;
		pt=ptnext;
		
		vector<cv::Point>::iterator itr = ptall.begin();

		//绘制完一个点后就移除出vector，否则会有可能重复绘制或者死循环
        while ( itr != ptall.end())
        {    
            if (ptnext == *itr)
            {
                ptall.erase(itr);
            }
            else
                ++itr;
        }
	}

	acedCommand ( RTSTR , _T("-BHATCH") , RTSTR , _T("P") , RTSTR , _T("SOLID") , RTSTR , _T("S") ,RTPICKS , sname , RTSTR , _T("") , RTSTR , _T("") ,RTNONE ) ;    //填充
	
	
	//PostToModelSpace(pPoly);
	/*
	//cv::Mat dxx;
	//cv::resize(edges,dxx,cv::Size(1000,1000));
	vector<cv::Vec4i> lines;
	cv::HoughLinesP(edges,lines,1,3.141592653/180,0,0,0);
	int count = lines.size();
	//AcDbPolyline *pPoly = new AcDbPolyline();
	AcGePoint3d ptSt(lines[0][0],lines[0][1],0);
	AcGePoint3d ptEnd(lines[0][2], lines[0][3],0);
	AcGePoint3d ptfirst((double(ptSt.x)+double(ptEnd.x))/2,(double(ptSt.y)+double(ptEnd.y))/2,0);
	AcGePoint3d ptSecond;

	for( size_t i = 1; i < lines.size(); i++ )
	{
		AcGePoint3d ptSt(lines[i][0],lines[i][1],0);
		AcGePoint3d ptEnd(lines[i][2], lines[i][3],0);
		ptSecond.x = (double(ptSt.x)+double(ptEnd.x))/2;
		ptSecond.y = (double(ptSt.y)+double(ptEnd.y))/2;
		ptSecond.z = 0;
		//pPoly->addVertexAt(i*2,ptSt,0,5,5);
		//pPoly->addVertexAt(i*2+1,ptEnd,0,5,5);
		AcDbLine *pLine = new AcDbLine(ptfirst,ptSecond);
		PostToModelSpace(pLine);

		ptfirst = ptSecond;
        //cv::line( edges, cv::Point(lines[i][0], lines[i][1]),cv::Point(lines[i][2], lines[i][3]), cv::Scalar(60,60,60), 1, 8 );
    }
	//pPoly->setClosed(Adesk::kTrue);
	//PostToModelSpace(pPoly);
	//cv::Mat dst;
	//cv::threshold(img,dst,128,255,cv::THRESH_BINARY);
	//vector<cv::Mat> channels;
	//cv::split(img,channels);
	//cv::ellipse(img,cv::Point(WINDOW_WIDTH/2,WINDOW_HEIGHT/2),cv::Size(WINDOW_WIDTH/2,WINDOW_HEIGHT/16),180,0,180,cv::Scalar(255,129,0));*/
	cv::namedWindow("image", CV_WINDOW_AUTOSIZE);  
	cv::resizeWindow("image",WINDOW_WIDTH,WINDOW_HEIGHT);
    cv::imshow("image",imggray);
	//cv::imshow("draw",edges);
	//cv::imshow("image2",end);
	//cv::imshow("image2",dst);
    //cv::imshow("1",channels.at(0));
    //cv::imshow("2",channels.at(1));
	//cv::imshow("3",channels.at(2));
    cv::waitKey();
}
//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class CConvertPicSignApp : public AcRxArxApp {

public:
	CConvertPicSignApp () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		// TODO: Load dependencies here

		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt) ;
		
		// TODO: Add your initialization code here
		acedRegCmds->addCommand(_T("SignPic"),_T("SignPic"),_T("SignPic"),ACRX_CMD_MODAL,Convert);
		return (retCode) ;
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		// TODO: Add your code here
		acedRegCmds->removeGroup(_T("SignPic"));
		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt) ;

		// TODO: Unload dependencies here

		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}
	
	// The ACED_ARXCOMMAND_ENTRY_AUTO macro can be applied to any static member 
	// function of the CConvertPicSignApp class.
	// The function should take no arguments and return nothing.
	//
	// NOTE: ACED_ARXCOMMAND_ENTRY_AUTO has overloads where you can provide resourceid and
	// have arguments to define context and command mechanism.
	
	// ACED_ARXCOMMAND_ENTRY_AUTO(classname, group, globCmd, locCmd, cmdFlags, UIContext)
	// ACED_ARXCOMMAND_ENTRYBYID_AUTO(classname, group, globCmd, locCmdId, cmdFlags, UIContext)
	// only differs that it creates a localized name using a string in the resource file
	//   locCmdId - resource ID for localized command

	// Modal Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CConvertPicSignApp, MyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL)
	static void MyGroupMyCommand () {
		// Put your command code here

	}

	// Modal Command with pickfirst selection
	// ACED_ARXCOMMAND_ENTRY_AUTO(CConvertPicSignApp, MyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET)
	static void MyGroupMyPickFirst () {
		ads_name result ;
		int iRet =acedSSGet (ACRX_T("_I"), NULL, NULL, NULL, result) ;
		if ( iRet == RTNORM )
		{
			// There are selected entities
			// Put your command using pickfirst set code here
		}
		else
		{
			// There are no selected entities
			// Put your command code here
		}
	}

	// Application Session Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CConvertPicSignApp, MyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION)
	static void MyGroupMySessionCmd () {
		// Put your command code here
	}

	// The ACED_ADSFUNCTION_ENTRY_AUTO / ACED_ADSCOMMAND_ENTRY_AUTO macros can be applied to any static member 
	// function of the CConvertPicSignApp class.
	// The function may or may not take arguments and have to return RTNORM, RTERROR, RTCAN, RTFAIL, RTREJ to AutoCAD, but use
	// acedRetNil, acedRetT, acedRetVoid, acedRetInt, acedRetReal, acedRetStr, acedRetPoint, acedRetName, acedRetList, acedRetVal to return
	// a value to the Lisp interpreter.
	//
	// NOTE: ACED_ADSFUNCTION_ENTRY_AUTO / ACED_ADSCOMMAND_ENTRY_AUTO has overloads where you can provide resourceid.
	
	//- ACED_ADSFUNCTION_ENTRY_AUTO(classname, name, regFunc) - this example
	//- ACED_ADSSYMBOL_ENTRYBYID_AUTO(classname, name, nameId, regFunc) - only differs that it creates a localized name using a string in the resource file
	//- ACED_ADSCOMMAND_ENTRY_AUTO(classname, name, regFunc) - a Lisp command (prefix C:)
	//- ACED_ADSCOMMAND_ENTRYBYID_AUTO(classname, name, nameId, regFunc) - only differs that it creates a localized name using a string in the resource file

	// Lisp Function is similar to ARX Command but it creates a lisp 
	// callable function. Many return types are supported not just string
	// or integer.
	// ACED_ADSFUNCTION_ENTRY_AUTO(CConvertPicSignApp, MyLispFunction, false)
	static int ads_MyLispFunction () {
		//struct resbuf *args =acedGetArgs () ;
		
		// Put your command code here

		//acutRelRb (args) ;
		
		// Return a value to the AutoCAD Lisp Interpreter
		// acedRetNil, acedRetT, acedRetVoid, acedRetInt, acedRetReal, acedRetStr, acedRetPoint, acedRetName, acedRetList, acedRetVal

		return (RTNORM) ;
	}
	
} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CConvertPicSignApp)

ACED_ARXCOMMAND_ENTRY_AUTO(CConvertPicSignApp, MyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CConvertPicSignApp, MyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CConvertPicSignApp, MyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION, NULL)
ACED_ADSSYMBOL_ENTRY_AUTO(CConvertPicSignApp, MyLispFunction, false)

