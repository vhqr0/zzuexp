from django.db import models
from django.db.models import Q
from django.core.mail import send_mail
from django.utils import timezone
from django.contrib.auth.models import User

import uuid
import datetime

from io import BytesIO
from PIL import Image


class UserInterface:

    @classmethod
    def create(cls, username, password, email):
        user = User.objects.create_user(username, email, password)
        user.save()

    @classmethod
    def set_password(cls, password, **kwargs):
        user = User.objects.get(**kwargs)
        user.set_password(password)
        user.save()

    @classmethod
    def get_username_by_email(cls, email):
        return User.objects.get(email=email).username

    @classmethod
    def email_count(cls, email):
        return User.objects.filter(email=email).count()

    @classmethod
    def username_or_email_count(cls, username, email):
        return User.objects.filter(Q(username=username)
                                   | Q(email=email)).count()


class VerifyRecord(models.Model):
    username = models.CharField(max_length=150)
    password = models.CharField(max_length=128)
    email = models.EmailField()
    verify_code = models.UUIDField()
    verify_type = models.CharField(max_length=10,
                                   choices=(('signin', 'Signin'),
                                            ('changepwd', 'Change Password')))
    send_time = models.DateTimeField(auto_now_add=True)
    is_active = models.BooleanField(default=True)

    def __str__(self):
        return self.email

    def save(self, *args, **kwargs):
        if not hasattr(self, 'verify_code') or not self.verify_code:
            self.verify_code = uuid.uuid4()
        if self.verify_type == 'changepwd':
            self.username = UserInterface.get_username_by_email(self.email)
        send_mail(f'Indexer Verify For {self.get_verify_type_display()}',
                  f'Username: {self.username}\nUUID: {self.verify_code}\n',
                  'www@indexer.net', [self.email])
        super().save(*args, **kwargs)

    def is_valid(self, verify_code):
        now = timezone.now()
        delta = datetime.timedelta(minutes=30)
        return self.is_active and \
            now - self.send_time < delta and \
            self.verify_code == verify_code

    def signin(self):
        self.is_active = False
        if self.verify_type == 'signin':
            assert UserInterface.username_or_email_count(
                self.username, self.email) == 0
            UserInterface.create(self.username, self.password, self.email)
        else:
            UserInterface.set_password(self.password,
                                       username=self.username,
                                       email=self.email)


class UserProfile(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    website = models.URLField(blank=True)
    self_introduction = models.TextField(max_length=1024, blank=True)

    def __str__(self):
        return self.user.username

    @classmethod
    def get_profile(cls, user):
        if hasattr(user, 'userprofile'):
            profile = user.userprofile
        else:
            profile = UserProfile(user=user)
            profile.save()
        return profile


class Avatar(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    data = models.BinaryField(max_length=4096)

    @classmethod
    def set(cls, user, img):
        bio = BytesIO()
        img = Image.open(img).resize((32, 32))
        img.save(bio, format='png')
        Avatar.objects.update_or_create(user=user,
                                        defaults={'data': bio.getvalue()})


class Category(models.Model):
    name = models.CharField(max_length=128, unique=True)
    views = models.IntegerField(default=0)
    likes = models.IntegerField(default=0)

    class Meta:
        verbose_name_plural = 'Categories'

    def __str__(self):
        return self.name


class Page(models.Model):
    category = models.ForeignKey(Category, on_delete=models.CASCADE)
    title = models.CharField(max_length=128)
    url = models.URLField()
    views = models.IntegerField(default=0)

    def __str__(self):
        return self.title
