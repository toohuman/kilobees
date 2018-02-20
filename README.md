# Kilobees
Kilobot implementation of distributed consensus algorithms based partially on time-modulation for site-quality dissemination similar to honeybees. The addition of a third truth state leads to improved robustness while retaining the same levels of accuracy as the standard Boolean version.

# Overview
This repository contains a collection of consensus algorithms developed originally for the Kilobox simulation environment for the Kilobots, before being rewritten in C for the Kilobots using the same Kilobot API. The repository includes Boolean and three-valued consensus algorithms presented at IROS 2017 (see [this](http://robohub.org/robust-distributed-decision-making-in-robot-swarms/) blog post for more details), as well as versions where a percentage of the population "malfunctions" by continuing to signal for a site at random. In this research we showed the following:

- A third truth state allows the swarm to reach the same levels of accuracy as in the Boolean model.
- The Boolean model (in which the updating robot simply adopts the signalling robot's opinion) is faster than the three-valued consensus model.
- The three-valued model is more robust to random agent malfunction for which the malfunctioning agents signal for a choice at random.
