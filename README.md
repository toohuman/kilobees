# kilobees
Kilobot implementation of distributed consensus algorithms based partially on time-modulation for site-quality dissemination similar to honeybees.

# Overview
This repository contains a collection of consensus algorithms developed originally for the Kilobox simulation environment for the Kilobots, before being rewritten in C for the Kilobots using the same Kilobot API. The repository includes Boolean and three-valued consensus algorithms, as well as versions where a percentage of the population "malfunctions" by continuing to signal for a site at random. Other types of malfunction should be explored including:
- Single-site signalling
- Worst-site signalling (malicious)
- Signalling actual beliefs for (random) time steps.
