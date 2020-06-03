docker-build:
	docker-compose build
	
openshift-applier/roles:
	cd openshift-applier/ && ansible-galaxy install -r requirements.yml --roles-path=roles

deploy: openshift-applier/roles
	cd openshift-applier/ && ansible-playbook -i inventory site.yml

.PHONY: docker-build openshift-applier/roles deploy