# Physiology Engine

## Purpose
The Physiology Engine provides deterministic context features for the APS research platform. It estimates insulin-on-board, carbohydrates-on-board, and simple activity-state features that can later be consumed by Predictor V2 and controller logic.

## IOB Model
The IOB module uses a simple linear decay curve over a bounded event buffer.

- Bolus and basal events are stored deterministically in a fixed-size ring buffer.
- Each event contributes full effect at insertion and decays linearly to zero over its configured action duration.
- Old events are expired once their contribution reaches zero.

This version is intentionally simple and predictable. It is not a clinical insulin model.

## COB Model
The COB module also uses a simple linear decay curve over a bounded meal buffer.

- Meal events are stored deterministically in a fixed-size ring buffer.
- Each meal decays linearly to zero over its configured absorption window.
- Absorbed meals are expired from the active set.

This version is intended for research context only.

## Activity Model
The activity module classifies motion from accelerometer samples.

- It computes motion above gravity from `ax/ay/az` inputs.
- It tracks rolling 5-minute and 15-minute motion RMS values.
- It produces a coarse activity state such as sedentary, light, moderate, exercise, or sleep.
- It maintains simple active and post-exercise minute counters.

## Limitations
- These models are deterministic and intentionally simple.
- They are not trained, personalized, or clinically validated.
- They do not directly command insulin.
- They should be treated as context signals only.

## Future Improvements
- Replace the linear decay approximations with better physiology models if research data justifies it.
- Add more nuanced activity inference if accelerometer and step-count data are available.
- Feed the outputs into Predictor V2 feature engineering once the export pipeline is ready.

## Predictor V2 Usage
Predictor V2 now consumes physiology context as structured input features:

- `iob_u`
- `cob_g`
- `activity_state`
- `activity_confidence_pct`
- `motion_rms_5m`
- `motion_rms_15m`
- `active_minutes`
- `post_exercise_minutes`
