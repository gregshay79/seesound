void DisplayWaveform(HDC hdc,int x,int y,int w,int h,double *data,int dataLen)
{
	HPEN greenPen;
	HGDIOBJ prevPen;
	int i,yval,yoff = h/2;
	double yscale = h/2./32768.;
	double xscale = ((double)(w))/dataLen;

	greenPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen

	// Erase previous
	PatBlt(hdc,x,y,w,h,BLACKNESS);
	//Draw bounding box and center line 
	prevPen= SelectObject(hdc,GetStockObject(WHITE_PEN));
	MoveToEx(hdc,x,y,NULL);
	LineTo(hdc,x+w,y);
	LineTo(hdc,x+w,y+h);
	LineTo(hdc,x,y+h);
	LineTo(hdc,x,y);
	MoveToEx(hdc,x,y+yoff,NULL);
	LineTo(hdc,x+w,y+yoff);
	// Display waveform
	SelectObject(hdc,greenPen);
	MoveToEx(hdc,x,y+yoff-((int)(data[0]*yscale)),NULL);
	for(i=0;i<dataLen;i++) {
		yval = (int)(data[i]*yscale);
		yval = yoff-yval;
		//SetPixel(hdc,i,(256+128)-yval,wcolor);
		LineTo(hdc,(int)(x+i*xscale),y+yval);
	}

	SelectObject(hdc,prevPen);
	DeleteObject(greenPen);
}

void DisplayScrolling(HDC hdc,int px,int py,int w,int h,double *data,int dlen)
{
	int i,yval;
	HGDIOBJ prevPen;
	double yscale;

	yscale = ((double)h)/dlen;
	
	//Draw bounding box
	prevPen=SelectObject(hdc,GetStockObject(WHITE_PEN));
	MoveToEx(hdc,px,py,NULL);
	LineTo(hdc,px+w,py);
	LineTo(hdc,px+w,py+h);
	LineTo(hdc,px,py+h);
	LineTo(hdc,px,py);

	//scroll to the left 1 pixel
	BitBlt(hdc,px+1,py+1,w-2,h-2,hdc,px+2,py+1,SRCCOPY );

	//SelectObject(hdc,GetStockObject(WHITE_PEN));
	//MoveToEx(hdc,0,768,NULL);

	for(i=0;i<dlen;i++) {
//		int x,y;
//		scrbuff[i]= .7* (10*log(/*sqrt*/(fbuffr[i]*fbuffr[i]+fbuffi[i]*fbuffi[i]))) + .3*scrbuff[i]; //FFT
		yval = (int)data[i]+ 64;
		//yval = (int)(dwin[i]*256);
		if (yval > 255) yval = 255;
		if (yval < 0) yval = 0;
		//MoveToEx(hdc,i+MATH_BUFFER_SIZE,256+128,NULL);
//		x = (i );
//		y = (256+512)-yval;
		SetPixel(hdc,px+w-2,py+h-1-((int)(i*yscale+0.5)),colortable[(int)yval]);
		//SetPixel(hdc,1023,1000-(i<<1)-1,colortable[(int)yval]);
		/*
		SelectObject(hdc,greenPen);
		LineTo(hdc,x,y);

		SelectObject(hdc,GetStockObject(BLACK_PEN));
		MoveToEx(hdc,x,y+1,NULL);
		LineTo(hdc,x,950);
		//MoveToEx(hdc,x-1,y+1,NULL);
		//ineTo(hdc,x-1,950);
		//SetPixel(hdc,x,y,wcolor);
		MoveToEx(hdc,x,y,NULL);
		//MoveToEx(hdc,i,256,NULL);
		//LineTo(hdc,i,256-yval);
		*/
	}

	SelectObject(hdc,prevPen);
}
