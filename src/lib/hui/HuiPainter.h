/*
 * HuiPainter.h
 *
 *  Created on: 25.06.2013
 *      Author: michi
 */

#ifndef HUIPAINTER_H_
#define HUIPAINTER_H_

class HuiWindow;

class HuiPainter
{
	public:
#ifdef HUI_API_GTK
	cairo_t *cr;
#endif
	HuiWindow *win;
	string id;
	int cur_font_size;
	string cur_font;
	bool cur_font_bold, cur_font_italic;
	void _cdecl End();
	color _cdecl GetThemeColor(int i);
	void _cdecl SetColor(const color &c);
	void _cdecl SetFont(const string &font, float size, bool bold, bool italic);
	void _cdecl SetFontSize(float size);
	void _cdecl SetAntialiasing(bool enabled);
	void _cdecl SetLineWidth(float w);
	void _cdecl DrawPoint(float x, float y);
	void _cdecl DrawLine(float x1, float y1, float x2, float y2);
	void _cdecl DrawLines(float *x, float *y, int num_lines);
	void _cdecl DrawLinesMA(Array<float> &x, Array<float> &y);
	void _cdecl DrawPolygon(float *x, float *y, int num_points);
	void _cdecl DrawPolygonMA(Array<float> &x, Array<float> &y);
	void _cdecl DrawRect(float x1, float y1, float w, float h);
	void _cdecl DrawRect(const rect &r);
	void _cdecl DrawCircle(float x, float y, float radius);
	void _cdecl DrawStr(float x, float y, const string &str);
	float _cdecl GetStrWidth(const string &str);
	void _cdecl DrawImage(float x, float y, const Image &image);
	int width, height;
};

#endif /* HUIPAINTER_H_ */