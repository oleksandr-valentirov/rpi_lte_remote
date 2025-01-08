<h1>Віддалене керування RPi5 по LTE</h1>
<p>Репозиторій містить набір утиліт для віддаленої взаємодії з RaspberryPi</p>

<h2>Збирання</h2>
<p>Для того щоб збілдити потрібну програму, виконується команда<br>
<code>make -C {dir name}</code><br>
де dir name це назва директорії з програмою.<br>
Наприклад:<br>
<code>make -C servo_cam</code></p>

<h1>Servo Cam</h1>
<p>Програма надає можливість отримувати відео з камери, підключеної до RPi, та керувати положенням камери двома сервоприводами. Для роботи також потрібні клієнт та сервер з білим айпі.</p>
<table>
    <thead>
        <tr>
            <th>Залежності</th>
            <th></th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>wiringPi</td>
            <td><a href=https://github.com/WiringPi/WiringPi/releases>link</a></td>
        </tr>
        <tr>
            <td>libconfuse</td>
            <td></td>
        </tr>
        <tr>
            <td>Клієнт та сервер</td>
            <td><a href=https://github.com/oleksandr-valentirov/remote_cam_app>link</a></td>
        </tr>
        <tr>
            <td>cam_subprocess</td>
            <td></td>
        </tr>
    </tbody>
</table>

<h1>Cam subprocess</h1>
<p>Програма захоплює кадри з CSI камери, та створює M-JPEG стрім на задану адресу.</p>
<table>
    <thead>
        <tr>
            <th>Залежності</th>
            <th></th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>libopencv-dev</td>
            <td></td>
        </tr>
    </tbody>
</table>