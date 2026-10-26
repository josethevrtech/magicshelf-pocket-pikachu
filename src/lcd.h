#pragma once
#include "emulator.h"
#include "render.h"
#include <QCoreApplication>
#include <memory>
// The 36 x 30 upstream pixels remain square. A separate upper strip carries
// the three menu indicators and the original step counter.
inline void drawLCD(Canvas& canvas, const QJSValue& frame) {
    canvas.fill(.68,.73,.52);
    static std::unique_ptr<cairo_surface_t,decltype(&cairo_surface_destroy)> background(
        cairo_image_surface_create_from_png((QCoreApplication::applicationDirPath()+"/../emulator/img/backgorundPikaSmall.png").toUtf8().constData()),cairo_surface_destroy);
    if(cairo_surface_status(background.get())==CAIRO_STATUS_SUCCESS){
        cairo_save(canvas.c);cairo_scale(canvas.c,double(canvas.w)/cairo_image_surface_get_width(background.get()),double(canvas.h)/cairo_image_surface_get_height(background.get()));
        cairo_set_source_surface(canvas.c,background.get(),0,0);cairo_paint_with_alpha(canvas.c,.45);cairo_restore(canvas.c);
    }
    // Transfer the complete LCD through Qt once, rather than 1080 JS property
    // lookups on the tracking/render thread.
    const auto pixels=frame.property("pixelLevels").toString();
    for(int y=0;y<30;y++)for(int x=0;x<36;x++) {
        const int n=y*36+x;const int level=n<pixels.size()?pixels[n].digitValue():0;
        if(level)canvas.rect(63+x*6.5,43+y*6.5,5.8,5.8,level==1?.48:.15,level==1?.54:.21,level==1?.36:.12);
    }
    // Native outlines retain the muted, reflective LCD appearance.
    for(int i=0;i<3;i++) {
        const bool active=frame.property("icons").property(i).toBool();
        double r=active?.15:.57,g=active?.21:.63,b=active?.12:.44;
        cairo_set_source_rgb(canvas.c,r,g,b);cairo_set_line_width(canvas.c,2.3);
        int x=36+i*31;
        cairo_save(canvas.c);cairo_translate(canvas.c,0,9);
        if(i==0){cairo_arc(canvas.c,x+10,17,9,0,2*M_PI);cairo_stroke(canvas.c);cairo_move_to(canvas.c,x+10,10);cairo_line_to(canvas.c,x+10,17);cairo_line_to(canvas.c,x+15,20);cairo_stroke(canvas.c);}
        if(i==1){cairo_rectangle(canvas.c,x+2,13,17,14);cairo_move_to(canvas.c,x,13);cairo_line_to(canvas.c,x+21,13);cairo_move_to(canvas.c,x+10,8);cairo_line_to(canvas.c,x+10,27);cairo_move_to(canvas.c,x+10,12);cairo_curve_to(canvas.c,x-4,-1,x+15,3,x+10,12);cairo_curve_to(canvas.c,x+24,-1,x+24,9,x+10,12);cairo_stroke(canvas.c);}
        if(i==2){for(int j=0;j<2;j++){cairo_arc(canvas.c,x+7+j*7,17-j*3,7,0,2*M_PI);cairo_stroke(canvas.c);}}
        cairo_restore(canvas.c);
    }
    // Seven-segment counter, right-aligned, without a system-font dependency.
    const unsigned digits[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
    auto number=frame.property("counter").toString().right(5);
    for(int i=0;i<number.size();i++) {
        int digit=number[i].digitValue();if(digit<0)continue;
        int x=324-(number.size()-i)*20,y=14;
        const int segments[7][4]={{3,0,10,2},{14,2,2,9},{14,13,2,9},{3,23,10,2},{0,13,2,9},{0,2,2,9},{3,11,10,2}};
        for(int s=0;s<7;s++)if(digits[digit]&(1<<s))canvas.rect(x+segments[s][0],y+segments[s][1],segments[s][2],segments[s][3],.15,.21,.12);
    }
}
