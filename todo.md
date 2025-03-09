1. Finished misc cases in global transform syncing

What to do?

1. revise what has changed and add testsok
1. add documents about various types of entity signal (the destroy ones in registry, holder and node are different), their lifetime and component access
1. write docs about what is allowed and not inside the signal handler
1. write docs about if it's allowed to directly use reg.destroy_entity