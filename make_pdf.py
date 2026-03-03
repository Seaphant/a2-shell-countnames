#!/usr/bin/env python3
"""Generate output.pdf from output_capture.txt for A2 submission."""
from reportlab.lib.pagesizes import letter
from reportlab.pdfgen import canvas
from reportlab.lib.units import inch

def main():
    with open('output_capture.txt', 'r') as f:
        lines = f.readlines()

    c = canvas.Canvas('output.pdf', pagesize=letter)
    width, height = letter
    margin = 0.75 * inch
    y = height - margin
    line_height = 12
    font_size = 10

    c.setFont('Courier', font_size)

    for line in lines:
        if y < margin:
            c.showPage()
            c.setFont('Courier', font_size)
            y = height - margin
        # Remove any non-printable chars
        text = line.rstrip('\n\r').encode('ascii', 'replace').decode()
        if len(text) > 100:
            text = text[:97] + '...'
        c.drawString(margin, y, text)
        y -= line_height

    c.save()
    print('Created output.pdf')

if __name__ == '__main__':
    main()
