#!/bin/sh
# $Id: run_atac.sh,v 1.1 1996/03/24 23:53:57 tom Exp $
rm -f /tmp/atac_dir/*
run_test.sh
atac -u ../*.atac /tmp/atac_dir/*
