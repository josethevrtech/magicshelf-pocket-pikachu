#include <QImage>
#include <QFile>
#include <QDataStream>
#include <iostream>
int main(int argc,char**argv){
 if(argc!=3)return 1;QImage image(argv[1]);if(image.isNull())return 2;
 if(image.width()>1024||image.height()>1024)image=image.scaled(1024,1024,Qt::KeepAspectRatio,Qt::SmoothTransformation);
 image=image.convertToFormat(QImage::Format_RGBA8888);
 QFile out(argv[2]);if(!out.open(QIODevice::WriteOnly))return 3;QDataStream data(&out);data.setByteOrder(QDataStream::LittleEndian);data<<quint32(image.width())<<quint32(image.height());
 // SDK texture alpha is an emission mask; this plastic housing emits no light.
 for(int y=0;y<image.height();y++){auto*p=image.scanLine(y);for(int x=0;x<image.width();x++)p[x*4+3]=0;if(out.write((char*)p,image.width()*4)!=image.width()*4)return 4;}
 std::cout<<image.width()<<" x "<<image.height()<<" housing texture\n";
}
