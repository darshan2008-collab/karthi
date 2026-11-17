@echo off
title Landslide Early Warning System Dashboard
echo ========================================================
echo   Landslide Early Warning & Monitoring Dashboard
echo   மண் சரிவு முன்னெச்சரிக்கை அமைப்பு
echo ========================================================
echo.
echo Starting Python Flask Web Server...
start "" http://localhost:5000
python app.py
pause
