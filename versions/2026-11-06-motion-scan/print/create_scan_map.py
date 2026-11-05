from pathlib import Path
from reportlab.pdfgen import canvas
from reportlab.lib.units import mm
from reportlab.lib.colors import black, Color
from pypdf import PdfReader

out = Path(__file__).with_name('A3_Scan_Test_Map.pdf')
c = canvas.Canvas(str(out), pagesize=(420*mm,297*mm), pageCompression=1)
c.setTitle('Kit32 A3 scan test map - 300 x 200 mm - actual size')
c.setAuthor('Kit32 project')
c.setSubject('Real black test shapes; external route guides only; print at 100 percent')
OX, OY = 60,45

def line(x1,y1,x2,y2,width=.18,gray=0):
    c.setStrokeColor(Color(gray,gray,gray));c.setLineWidth(width*mm)
    c.line(x1*mm,y1*mm,x2*mm,y2*mm)

def text(x,y,s,size=8,align='left',gray=0,font='Helvetica'):
    c.setFillColor(Color(gray,gray,gray));c.setFont(font,size)
    fn={'left':c.drawString,'center':c.drawCentredString,'right':c.drawRightString}[align]
    fn(x*mm,y*mm,s)

def arrow(x1,y1,x2,y2,gray=.4):
    import math
    line(x1,y1,x2,y2,.23,gray)
    a=math.atan2(y2-y1,x2-x1)
    for side in (-1,1):
        q=a+side*.55
        line(x2,y2,x2-2*math.cos(q),y2-2*math.sin(q),.23,gray)

text(18,282,'KIT32 / SCAN TEST MAP',17,font='Helvetica-Bold')
text(402,283,'A3 LANDSCAPE  /  1:1  /  MILLIMETRES',9,'right')
text(18,273,'300 x 200 mm scan field  |  7 lanes at 50 mm  |  Origin at lower left',9,gray=.3)

# Real test targets. No route line or grid is printed inside the sensing field.
c.setFillColor(black)
c.rect((OX+65)*mm,(OY+40)*mm,80*mm,60*mm,stroke=0,fill=1)
c.rect((OX+40)*mm,(OY+130)*mm,20*mm,45*mm,stroke=0,fill=1)
c.circle((OX+240)*mm,(OY+145)*mm,32*mm,stroke=0,fill=1)
p=c.beginPath();p.moveTo((OX+155)*mm,(OY+40)*mm)
p.lineTo((OX+205)*mm,(OY+110)*mm);p.lineTo((OX+245)*mm,(OY+40)*mm);p.close()
c.drawPath(p,stroke=0,fill=1)

# Outward-facing corner marks: no black border touching the scan endpoints.
for xx,yy,sx,sy in [(OX,OY,-1,-1),(OX+300,OY,1,-1),(OX,OY+200,-1,1),(OX+300,OY+200,1,1)]:
    line(xx+sx*2,yy+sy*2,xx+sx*7,yy+sy*2,.18,.4)
    line(xx+sx*2,yy+sy*2,xx+sx*2,yy+sy*7,.18,.4)

# Lane labels and motion arrows are outside the measured field.
for lane in range(7):
    x=OX+lane*50
    text(x,263,f'L{lane+1}  /  x={lane*50}',8,'center',.25,'Helvetica-Bold')
    if lane%2==0:
        arrow(x,OY-11,x,OY-2)
    else:
        arrow(x,OY+211,x,OY+202)
    if lane<6:
        y=OY+209 if lane%2==0 else OY-9
        arrow(x+3,y,x+47,y)

text(OX-9,OY-13,'START',8,'right',0,'Helvetica-Bold')
text(OX-9,OY-17,'(0, 0)',7,'right',.3)
text(OX+300+9,OY+202,'FINISH',8,'left',0,'Helvetica-Bold')
text(OX+300+9,OY+197,'(300, 200)',7,'left',.3)

# Overall dimensions outside the field.
line(43,OY,43,OY+200,.15,.5)
for yy in [OY,OY+200]:line(40,yy,46,yy,.18,.4)
c.saveState();c.translate(38*mm,145*mm);c.rotate(90)
c.setFont('Helvetica',8);c.setFillColor(Color(.3,.3,.3));c.drawCentredString(0,0,'200 mm scan travel');c.restoreState()
line(OX,22,OX+300,22,.15,.5)
for lane in range(7):line(OX+lane*50,20.5,OX+lane*50,23.5,.15,.4)
text(OX+150,25,'300 mm width  /  6 shifts x 50 mm',8,'center',.3)

# Notes and reference black patch remain well away from the measured field.
text(18,235,'BLACK',7,'center',.25,'Helvetica-Bold')
c.setFillColor(black);c.rect(11*mm,215*mm,14*mm,14*mm,fill=1,stroke=0)
for y,s in [(188,'No grid'),(183,'or route ink'),(178,'inside field.'),(163,'Guides are'),(158,'outside the'),(153,'scan area.')]:text(18,y,s,6.5,'center',.4)

text(374,167,'REAL TEST',7,font='Helvetica-Bold')
text(374,162,'TARGETS',7,font='Helvetica-Bold')
for y,s in [(150,'Solid black shapes'),(145,'on a white surface.'),(132,'Robot reference'),(127,'point starts at'),(122,'the lower-left'),(117,'field corner.'),(104,'Leave room for'),(99,'the robot body'),(94,'and turns.'),(81,'Open-loop motion'),(76,'may deviate from'),(71,'the planned lanes.')]:text(374,y,s,6.5,gray=.3)

# A genuinely dimensioned 100 mm ruler to verify printer scaling.
line(60,9,160,9,.25)
for i in range(11):line(60+i*10,9,60+i*10,9+(3 if i%5==0 else 1.5),.2)
text(110,14,'PRINT CHECK: 100 mm',7,'center',0,'Helvetica-Bold')
text(180,12,'PRINT AT ACTUAL SIZE / 100% - DO NOT FIT TO PAGE',8,font='Helvetica-Bold')
text(180,7,'Measure the ruler before testing. Use flat, matte paper; avoid folds and glare.',7,gray=.3)
c.showPage();c.save()
r=PdfReader(str(out));assert len(r.pages)==1
w,h=map(float,(r.pages[0].mediabox.width,r.pages[0].mediabox.height))
assert abs(w/mm-420)<.001 and abs(h/mm-297)<.001
print(f'Created {out}; one A3 page, {w/mm:.3f} x {h/mm:.3f} mm; scan field 300 x 200 mm; calibration ruler 100 mm.')
