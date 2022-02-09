-- phpMyAdmin SQL Dump
-- version 4.1.4
-- http://www.phpmyadmin.net
--
-- Host: 127.0.0.1
-- Generation Time: Apr 27, 2021 alle 23:00
-- Versione del server: 5.6.15-log
-- PHP Version: 5.5.8

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `avedb`
--
CREATE DATABASE IF NOT EXISTS `avedb` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;
USE `avedb`;
 
DROP TABLE IF EXISTS `men_users`;

DROP TABLE IF EXISTS `users`;

DROP TABLE IF EXISTS `clients_men`;

DROP TABLE IF EXISTS `men`;

DROP TABLE IF EXISTS `measures`;

DROP TABLE IF EXISTS `configs`;

DROP TABLE IF EXISTS `ranges`;

DROP TABLE IF EXISTS `controllers`;

DROP TABLE IF EXISTS `clients`;

DROP TABLE IF EXISTS `devices`;

-- --------------------------------------------------------

--
-- Struttura della tabella `devices`
--

CREATE TABLE `devices` (
  `device` varchar(1) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `name` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `sensors` tinyint(4) NOT NULL,
  PRIMARY KEY (`device`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Svuota la tabella prima dell'inserimento `devices`
--

TRUNCATE TABLE `devices`;
--
-- Dump dei dati per la tabella `devices`
--

INSERT INTO `devices` (`device`, `name`, `sensors`) VALUES
('A', 'ave_Ambient', 2),
('M', 'ave_Monitor', 3);

-- --------------------------------------------------------

--
-- Struttura della tabella `clients`
--

CREATE TABLE `clients` (
  `client` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `topic` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `device` varchar(1) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  PRIMARY KEY (`client`),
  KEY `device` (`device`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Svuota la tabella prima dell'inserimento `clients`
--

TRUNCATE TABLE `clients`;
--
-- Dump dei dati per la tabella `clients`
--

INSERT INTO `clients` (`client`, `topic`, `device`) VALUES
('ave_Ambient', 'ave_Ambient', 'A'),
('ave_Monitor', 'ave_Monitor', 'M'),
('ave_Ambient001', 'ave_Ambient001', 'A'),
('ave_Monitor001', 'ave_Monitor001', 'M'),
('ave_Monitor002', 'ave_Monitor002', 'M'),
('ave_Monitor003', 'ave_Monitor003', 'M');

-- --------------------------------------------------------

--
-- Struttura della tabella `controllers`
--

CREATE TABLE `controllers` (
  `mac` varchar(17) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `client` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `name` varchar(25) NOT NULL,
  PRIMARY KEY (`mac`),
  KEY `client` (`client`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Svuota la tabella prima dell'inserimento `controllers`
--

TRUNCATE TABLE `controllers`;
--
-- Dump dei dati per la tabella `controllers`
--

INSERT INTO `controllers` (`mac`, `client`, `name`) VALUES
('24:6F:28:60:69:D8', 'ave_Ambient001', 'ESP32'),
('24:6F:28:60:6D:A0', 'ave_Monitor001', 'ESP32'),
('24:6F:28:60:68:C4', 'ave_Monitor002', 'ESP32'),
('24:6F:28:60:6F:3C', 'ave_Monitor003', 'ESP32');

-- --------------------------------------------------------

--
-- Struttura della tabella `configs`
--

CREATE TABLE `configs` (
  `code` int(11) NOT NULL AUTO_INCREMENT,
  `client` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `number` tinyint(4) NOT NULL,
  `refresh` tinyint(1) NOT NULL,
  `sensor` varchar(20) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `input` tinyint(4) NOT NULL,
  `inhibitor` tinyint(4) NOT NULL,
  `function` varchar(1) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `average` tinyint(4) NOT NULL,
  `sample` smallint(6) NOT NULL,
  `pointMin` smallint(6) NOT NULL,
  `pointMax` smallint(6) NOT NULL,
  `valueMin` smallint(6) NOT NULL,
  `valueMax` smallint(6) NOT NULL,
  `period` smallint(6) NOT NULL,
  `decimals` tinyint(4) NOT NULL,
  PRIMARY KEY (`code`),
  KEY `client` (`client`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=6 ;

--
-- Svuota la tabella prima dell'inserimento `configs`
--

TRUNCATE TABLE `configs`;
--
-- Dump dei dati per la tabella `configs`
--

INSERT INTO `configs` (`code`, `client`, `number`, `refresh`, `sensor`, `input`, `inhibitor`, `function`, `average`, `sample`, `pointMin`, `pointMax`, `valueMin`, `valueMax`, `period`, `decimals`) VALUES
(1, 'ave_Ambient', 0, 0, 'Temperature', 34, -1, 'L', 16, 5000, 186, 2826, -450, 1150, 0, 1),
(2, 'ave_Ambient', 1, 0, 'Humidity', 35, -1, 'L', 16, 5000, 186, 3066, 0, 100, 0, 0),
(3, 'ave_Monitor', 0, 0, 'Temperature', 34, 2, 'L', 16, 1000, 130, 426, 250, 500, 0, 1),
(4, 'ave_Monitor', 1, 0, 'Heart Rate', 35, 2, 'F', 8, 2, 500, 3500, 300, 2000, 60, 0),
(5, 'ave_Monitor', 2, 0, 'Nothing', 36, -1, 'L', 4, 1000, 0, 1024, 0, 1, 0, 0);

-- --------------------------------------------------------

--
-- Struttura della tabella `ranges`
--

CREATE TABLE `ranges` (
  `code` int(11) NOT NULL AUTO_INCREMENT,
  `client` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `number` tinyint(4) NOT NULL,
  `refresh` tinyint(1) NOT NULL,
  `startdate` date,
  `enddate` date,
  `wrnMin` smallint(6) NOT NULL,
  `wrnMax` smallint(6) NOT NULL,
  `dngMin` smallint(6) NOT NULL,
  `dngMax` smallint(6) NOT NULL,
  `intNrm` smallint(6) NOT NULL,
  `intWrn` smallint(6) NOT NULL,
  `intDng` smallint(6) NOT NULL,
  `actWrn` tinyint(4) NOT NULL,
  `actDng` tinyint(4) NOT NULL,
  PRIMARY KEY (`code`),
  KEY `client` (`client`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=6 ;

--
-- Svuota la tabella prima dell'inserimento `ranges`
--

TRUNCATE TABLE `ranges`;
--
-- Dump dei dati per la tabella `ranges`
--

INSERT INTO `ranges` (`code`, `client`, `number`, `refresh`, `wrnMin`, `wrnMax`, `dngMin`, `dngMax`, `intNrm`, `intWrn`, `intDng`, `actWrn`, `actDng`) VALUES
(1, 'ave_Ambient', 0, 0, -100, 300, -200, 400, 60, 30, 10, 1, 1),
(2, 'ave_Ambient', 1, 0, 20, 70, 10, 80, 60, 30, 10, 1, 1),
(3, 'ave_Monitor', 0, 0, 360, 380, 350, 390, 60, 30, 10, 0, 0),
(4, 'ave_Monitor', 1, 0, 60, 100, 50, 150, 60, 30, 10, 0, 0),
(5, 'ave_Monitor', 2, 0, 0, 4, 0, 4, 0, 0, 0, 0, 0);

-- --------------------------------------------------------

--
-- Struttura della tabella `measures`
--

CREATE TABLE `measures` (
  `code` int(11) NOT NULL AUTO_INCREMENT,
  `client` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `sensor` varchar(20) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `range` varchar(10) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `value` float NOT NULL,
  `date` date NOT NULL,
  `time` time NOT NULL,
  PRIMARY KEY (`code`),
  KEY `client` (`client`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Svuota la tabella prima dell'inserimento `measures`
--

TRUNCATE TABLE `measures`;
-- --------------------------------------------------------

--
-- Struttura della tabella `men`
--

CREATE TABLE `men` (
  `code` int(11) NOT NULL AUTO_INCREMENT,
  `surname` varchar(50) NOT NULL,
  `name` varchar(30) NOT NULL,
  `weight` tinyint(4) NOT NULL,
  `height` tinyint(4) NOT NULL,
  `birthdate` date NOT NULL,
  PRIMARY KEY (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Svuota la tabella prima dell'inserimento `men`
--

TRUNCATE TABLE `men`;
-- --------------------------------------------------------

--
-- Struttura della tabella `clients_men`
--

CREATE TABLE `clients_men` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `client` varchar(15) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `code` int(11) NOT NULL,
  `startdate` date NOT NULL,
  `enddate` date,
  PRIMARY KEY (`id`),
  KEY `client` (`client`),
  KEY `code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Svuota la tabella prima dell'inserimento `clients_men`
--

TRUNCATE TABLE `clients_men`;
-- --------------------------------------------------------

--
-- Struttura della tabella `users`
--

CREATE TABLE `users` (
  `username` varchar(20) NOT NULL,
  `password` varchar(32) NOT NULL,
  `email` varchar(50) NOT NULL,
  `surname` varchar(50) NOT NULL,
  `name` varchar(30) NOT NULL,
  `role` tinyint(4) NOT NULL,
  PRIMARY KEY (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Svuota la tabella prima dell'inserimento `users`
--

TRUNCATE TABLE `users`;
-- --------------------------------------------------------

--
-- Struttura della tabella `men_users`
--

CREATE TABLE `men_users` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `code` int(11) NOT NULL,
  `username` varchar(20) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `code` (`code`),
  KEY `username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Svuota la tabella prima dell'inserimento `men_users`
--

TRUNCATE TABLE `men_users`;
--
-- Limiti per le tabelle scaricate
--

--
-- Limiti per la tabella `clients`
--
ALTER TABLE `clients`
  ADD CONSTRAINT `clients_ibfk_1` FOREIGN KEY (`device`) REFERENCES `devices` (`device`) ON DELETE NO ACTION ON UPDATE CASCADE;

--
-- Limiti per la tabella `controllers`
--
ALTER TABLE `controllers`
  ADD CONSTRAINT `controllers_ibfk_1` FOREIGN KEY (`client`) REFERENCES `clients` (`client`) ON DELETE NO ACTION ON UPDATE CASCADE;

--
-- Limiti per la tabella `configs`
--
ALTER TABLE `configs`
  ADD CONSTRAINT `configs_ibfk_1` FOREIGN KEY (`client`) REFERENCES `clients` (`client`) ON DELETE NO ACTION ON UPDATE CASCADE;

--
-- Limiti per la tabella `ranges`
--
ALTER TABLE `ranges`
  ADD CONSTRAINT `ranges_ibfk_1` FOREIGN KEY (`client`) REFERENCES `clients` (`client`) ON DELETE NO ACTION ON UPDATE CASCADE;

--
-- Limiti per la tabella `measures`
--
ALTER TABLE `measures`
  ADD CONSTRAINT `measures_ibfk_1` FOREIGN KEY (`client`) REFERENCES `clients` (`client`) ON DELETE NO ACTION ON UPDATE CASCADE;

--
-- Limiti per la tabella `client_men`
--
ALTER TABLE `clients_men`
  ADD CONSTRAINT `clients_men_ibfk_1` FOREIGN KEY (`client`) REFERENCES `clients` (`client`) ON DELETE NO ACTION ON UPDATE CASCADE,
  ADD CONSTRAINT `clients_men_ibfk_2` FOREIGN KEY (`code`) REFERENCES `men` (`code`) ON DELETE NO ACTION ON UPDATE CASCADE;

--
-- Limiti per la tabella `men_users`
--
ALTER TABLE `men_users`
  ADD CONSTRAINT `men_users_ibfk_1` FOREIGN KEY (`code`) REFERENCES `men` (`code`) ON DELETE NO ACTION ON UPDATE CASCADE,
  ADD CONSTRAINT `men_users_ibfk_2` FOREIGN KEY (`username`) REFERENCES `users` (`username`) ON DELETE NO ACTION ON UPDATE CASCADE;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
