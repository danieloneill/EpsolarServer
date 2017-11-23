-- MySQL dump 10.13  Distrib 5.5.52, for debian-linux-gnu (armv7l)
--
-- Host: localhost    Database: epsolar
-- ------------------------------------------------------
-- Server version	5.5.52-0+deb8u1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `epsolar`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `epsolar` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `epsolar`;

--
-- Table structure for table `fiveMinute`
--

DROP TABLE IF EXISTS `fiveMinute`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fiveMinute` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `register` int(11) DEFAULT NULL,
  `min` decimal(8,2) DEFAULT NULL,
  `max` decimal(8,2) DEFAULT NULL,
  `average` decimal(8,2) DEFAULT NULL,
  `tstart` datetime DEFAULT NULL,
  `tend` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `hourly`
--

DROP TABLE IF EXISTS `hourly`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hourly` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `register` int(11) DEFAULT NULL,
  `min` decimal(8,2) DEFAULT NULL,
  `max` decimal(8,2) DEFAULT NULL,
  `average` decimal(8,2) DEFAULT NULL,
  `tstart` datetime DEFAULT NULL,
  `tend` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `registers`
--

DROP TABLE IF EXISTS `registers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `registers` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `register` int(11) DEFAULT NULL,
  `name` varchar(32) DEFAULT NULL,
  `measure` varchar(8) DEFAULT NULL,
  `scale` double(8,3) DEFAULT NULL,
  `multibyte` enum('SOLE','LOW','HIGH','BITMAP') DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

LOCK TABLES `registers` WRITE;
/*!40000 ALTER TABLE `registers` DISABLE KEYS */;
INSERT INTO `registers` VALUES (1,12544,'Charge voltage','V',0.010,'SOLE'),(2,12545,'Charge current','A',0.010,'SOLE'),(3,12546,'Charge watts','W',0.010,'SOLE'),(4,12556,'Load voltage','V',0.010,'SOLE'),(5,12557,'Load current','A',0.010,'SOLE'),(6,12558,'Load watts','W',0.010,'SOLE'),(7,12800,'Battery status',NULL,1.000,'BITMAP'),(8,12801,'Charge controller status',NULL,1.000,'BITMAP'),(9,12570,'Battery SOC','%',1.000,'SOLE'),(10,12573,'Battery temperature','C',0.010,'SOLE'),(11,13060,'Consumed energy today','Wh',100.000,'LOW'),(12,13061,'Consumed energy today','Wh',100.000,'HIGH'),(13,13068,'Generated energy today','Wh',100.000,'LOW'),(14,13069,'Generated energy today','Wh',100.000,'HIGH'),(15,13074,'Generated energy total','KWh',0.010,'LOW'),(16,13075,'Generated energy total','KWh',0.010,'HIGH'),(17,13076,'CO2 reduction','Kg',100.000,'LOW'),(18,13077,'CO2 reduction','Kg',100.000,'HIGH');
/*!40000 ALTER TABLE `registers` ENABLE KEYS */;
UNLOCK TABLES;

-- Dump completed on 2017-06-11 22:57:55

