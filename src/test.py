import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

sender_email = "s6503014622162@email.kmutnb.ac.th"
app_password = "dwkk adab imbd pqmh" 
receiver_email = "skyki004@hotmail.com"

subject = "Alert ESP32"
body = "ESP32 Alert!"

msg = MIMEMultipart()
msg['From'] = sender_email
msg['To'] = receiver_email
msg['Subject'] = subject

msg.attach(MIMEText(body, 'plain'))

try:
    server = smtplib.SMTP("smtp.gmail.com", 587)
    server.starttls()
    server.login(sender_email, app_password)
    server.sendmail(sender_email, receiver_email, msg.as_string())
    server.quit()
    print("âœ… Email Sent Successfully")
except Exception as e:
    print("âŒ Failed:", e)