Copy these files into:
~/Desktop/Nutrient Stress/raspberry_pi_hat_inference_only

Run:
.venv/bin/python predict_and_log.py --json test_healthy.json
.venv/bin/python predict_and_log.py --json test_moderate.json
.venv/bin/python predict_and_log.py --json test_high.json

The trained HAT determines the actual prediction. The filenames only describe
the intended test condition and do not guarantee a particular class.
