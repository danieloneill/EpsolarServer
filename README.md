# Tracer series RS485/MODBUS monitoring software for Linux (and Raspberry Pi I guess)

![Screenshot of EpsolarServer's web view](http://i.imgur.com/LQi7Q6S.png)

You'll need a few things to build this:
* Linux
* Qt 5.5+ ("qtbase5-dev", "qt5-qmake", "libqt5sql5", "qt5-default", and probably "libqt5sql5-mysql")
* MySQL/Mariadb ("mysql-client", "mysql-server")

If you want to monitor your stats with the integrated websocket server: *(If you skip this, the web interface won't work!)*
* QtWebsockets ("libqt5websockets5-dev")

If you want to use libmodbus (probably Yes if on a raspberry pi):
* libmodbus5
* libmodbus-dev

Credit to: zlib.js, graph.js, qhttpserver, and the listed software above.

## Building

If you want to use the integrated web server, you must build and install "qhttpserver" located in "3rdparty/" first. You need the Qt dependencies mentioned above, but otherwise just "qmake", "make", and "make install" this.

If you don't want to use the integrated web server, install the webserver of your choice and copy all the files in "www/" to your web server document root.

To set build options, see the top options in "EpsolarServer.pro".

Build by running "qmake", then "make".

## Setup

To setup the database, import the schema using "mysql -uroot < dist/mysql_schema.sql"

To configure, copy "dist/epsolarServer.conf" to "/etc/" and edit the copy.

The meanings are as follows:

```
databaseType: Can be QMYSQL, QPGSQL, QSQLITE, and so on, but only QMYSQL is tested or known to work.
databaseName: The database name
databaseHostname: The database hostname
databaseUsername: The database account username
databasePassword: The database account password (if any)
epsolarDevicePath: The file path to your RS485 adapter character device node (usually /dev/ttyBLAHBLAH0 or something)
epsolarPollFrequencyMS: How long the should server pause between reading registers in milliseconds. I suggest no less than 20 here.
```

To start the software when your system boots up, edit "epsolar.init" and copy it to "/etc/init.d/epsolar". Then run "update-rc.d epsolar defaults".

## Running

### Web Interface
Once running, you can see your graph start to populate by pointing your HTML5 compliant web browser to:

*http://(host address):8080/*

The top graph lays out 5-minute averages. The bottom graph lays out hourly averages.

Any gaps of white in the graph indicate that the EpsolarServer program wasn't running for that period, such as a power outage or battery bank maintenance, or the disk is full and no additional records can be saved.

### Websocket
If you aren't interested at all in the web interface, you can interact with the websocket interface as follows:

*ws://(host address):7175/*

The server expects JSON formatted requests as text frames, and responds with gzipped JSON responses as binary frames or plain-text JSON depending on if compression is enabled on the connection.

Specifying "compress" with either a true or false value will enable or disable GZip compression on ALL subsequent responses, until it is specified in a new request.

Valid requests:

* Averages: **5-minute average records** which are compressed into hourly averages once 24-hours old.
```
	Request:
	{
                'action': 'averages',
                'from': <int, earliest timestamped record to fetch, in unix epoch milliseconds>,
                'to': <int, latest timestamped record to fetch, in unix epoch milliseconds>,
                'count': <int, how many records to fetch>,
                'register': <int, register ID>,
		'compress': <true/false, for GZip compressed responses>
        }

	Response:
	{
		"data": {
			"Charge watts": [
				{
					"avg": 0,
					"end": 1497160419000,
					"max": 0,
					"min": 0,
					"start": 1497160119000
				},
				{
					"avg": 0,
					"end": 1497160720000,
					"max": 0,
					"min": 0,
					"start": 1497160419000
				},
				...
			]
		},
		"type": "averages"
	}
```

* Hourlies: **60-minute average records** which will accrue ad-infinitum.
```
	Request:
	{
                'action': 'hourly',
                'from': <int, earliest timestamped record to fetch, in unix epoch milliseconds>,
                'to': <int, latest timestamped record to fetch, in unix epoch milliseconds>,
                'count': <int, how many records to fetch>,
                'register': <int, register ID>,
		'compress': <true/false, for GZip compressed responses>
        }

	Response (example):
	{
		"data": {
			"Load voltage": [
				{
					"avg": 13.859999999999999,
					"date": "2017-06-05",
					"hour": 19,
					"max": 13.970000000000001,
					"min": 13.779999999999999
				},
				{
					"avg": 13.9,
					"date": "2017-06-05",
					"hour": 20,
					"max": 14.220000000000001,
					"min": 13.529999999999999
				},
				...
			]
 		},
		"type": "hourly"
	}
```

* Subscribe: **Records sent every (register count x epsolarPollFrequencyMS)ms interval** This is effectively real-time readings.
```
	Request:
	{
		'action': 'subscribe',
		'compress': <true/false, for GZip compressed responses>,
		'subscribe': <true/false, true to subscribe, false to unsubscribe>
	}

	Response (example):
	{
		"type": "reading",
		"data": {
			"Battery SOC": 29,
			"Battery status": 0,
			"Battery temperature": 12,
			"CO2 reduction": 400,
			"Charge controller status": 1,
			"Charge current": 0,
			"Charge voltage": 1.4099999999999999,
			"Charge watts": 0,
			"Consumed energy today": 0,
			"Generated energy today": 700,
			"Generated energy total": 47,
			"Load current": 0,
			"Load voltage": 12.83,
			"Load watts": 0
		}
	}
	... etc ... it never stops ...

```
