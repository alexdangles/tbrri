#!/bin/sh
root -l 'GenerateGateConfiguration.C( "Gamma_Camera.txt" )'
Gate Gamma_Camera.mac
root -l Gamma_Camera.C
